#include "fmMod.h"
#include <string.h>
#include <mutex>
#include <iostream>
#include "waveRead.cpp"
#include "algorithm.cpp"

using namespace std;

//_ means global variable

int _audioSampleRate=0;
float * _audioSampleBuf=NULL;
float * _new_audio_buf=NULL;
unsigned int offset=0;
int _hackrfSampleRate=2000000;
int32_t  _numSampleCount;
int8_t ** _iqCache;
int _buffCount = 0;


void getPcmData(char *path){

    WaveData *wave = wavRead(path, strlen(path));
    int nch = wave->header.numChannels;
    _audioSampleRate=wave->sampleRate;
    _numSampleCount = wave->size / wave->header.blockAlign;

    cout<<_numSampleCount<<endl;
    _audioSampleBuf=new float[_numSampleCount]();
    _new_audio_buf = new float[BUF_LEN/2]();

    if(nch==1){

        for(int i=0;i<_numSampleCount;i++){

            _audioSampleBuf[i] = wave->samples[i];
        }

    }else if(nch==2){

            for(int i=0;i<_numSampleCount;i++){

                _audioSampleBuf[i] = (wave->samples[i * 2] + wave->samples[i * 2 + 1]) / (float)2.0;
        }
    }

}

void makeCache() {
    int _nsample = (float)_audioSampleRate * (float)BUF_LEN / (float)_hackrfSampleRate / 2.0;


    _iqCache = new int8_t*[_numSampleCount / _nsample]();
    for(int i = 0; i < _numSampleCount / _nsample; i++) {
        _iqCache[i] = new int8_t[BUF_LEN]();
    }

    for(int i = 0; i < _numSampleCount / _nsample; i++) {
        interpolation(_audioSampleBuf + (_nsample * i), _nsample, _new_audio_buf, BUF_LEN / 2);
        modulation(_new_audio_buf, _iqCache[i], 1);
    }

}


int hackrf_tx_callback(int8_t *buffer, uint32_t length) {
    int _nsample = (float)_audioSampleRate * (float)BUF_LEN / (float)_hackrfSampleRate / 2.0;
    if(_buffCount <= (_numSampleCount / _nsample)) {
        memcpy(buffer, _iqCache[_buffCount], length);
        _buffCount++;
    }
    return 0;
}


int _hackrf_tx_callback(hackrf_transfer *transfer) {
    return hackrf_tx_callback((int8_t *)transfer->buffer, transfer->valid_length);
}


void hackrfWork() {
    double freq = 433.00 * 1000000;
    uint32_t gain = 90 / 100.0;
    uint32_t tx_vga = 40;
    uint32_t enableamp = 1;
    hackrf_init();
    hackrf_device * _dev = NULL;
    int ret = hackrf_open(&_dev);

    if (ret != HACKRF_SUCCESS) {
        hackrf_close(_dev);
        cout<<"hackrf open error"<<endl;
    }
    else {
        hackrf_set_sample_rate(_dev, _hackrfSampleRate);
        uint32_t bw=hackrf_compute_baseband_filter_bw_round_down_lt(_hackrfSampleRate);
        hackrf_set_baseband_filter_bandwidth(_dev, bw);
        hackrf_set_freq(_dev, freq);
        hackrf_set_txvga_gain(_dev, tx_vga);
        hackrf_set_amp_enable(_dev, enableamp);
        ret = hackrf_start_tx(_dev, _hackrf_tx_callback, NULL);
        if (ret != HACKRF_SUCCESS) {
            hackrf_close(_dev);
            cout<<"hackrf transfer error"<<endl;
        }
    }
}
