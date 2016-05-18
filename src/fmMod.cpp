#include "fmMod.h"
#include <math.h>
#include <string.h>
#include <mutex>
#include <iostream>
#include "wavOperator.cpp"

using namespace std;

int _audioSampleRate=0;
float * _audioSampleBuf=NULL;
float * _new_audio_buf=NULL;
unsigned int offset=0;
int _hackrfSampleRate=2000000;
int32_t  numSampleCount;
int8_t ** iqCache;
int xcount = 0;


void interpolation(float * in_buf, uint32_t in_samples, float * out_buf, uint32_t out_samples) {

    float last_in_samples[4] = { 0.0, 0.0, 0.0, 0.0 };
    uint32_t i;		/* Input buffer index + 1. */
    uint32_t j = 0;	/* Output buffer index. */
    float pos;		/* Position relative to the input buffer
						* + 1.0. */

    /* We always "stay one sample behind", so what would be our first sample
    * should be the last one wrote by the previous call. */
    pos = (float)in_samples / (float)out_samples;
    while (pos < 1.0)
    {
        out_buf[j] = last_in_samples[3] + (in_buf[0] - last_in_samples[3]) * pos;
        j++;
        pos = (float)(j + 1)* (float)in_samples / (float)out_samples;
    }

    /* Interpolation cycle. */
    i = (uint32_t)pos;
    while (j < (out_samples - 1))
    {

        out_buf[j] = in_buf[i - 1] + (in_buf[i] - in_buf[i - 1]) * (pos - (float)i);
        j++;
        pos = (float)(j + 1)* (float)in_samples / (float)out_samples;
        i = (uint32_t)pos;
    }

    /* The last sample is always the same in input and output buffers. */
    out_buf[j] = in_buf[in_samples - 1];

    /* Copy last samples to last_in_samples (reusing i and j). */
    for (i = in_samples - 4, j = 0; j < 4; i++, j++)
        last_in_samples[j] = in_buf[i];
}


void modulation(float * input, int8_t * output, uint32_t mode) {
    double fm_deviation = NULL;
    float gain = 0.9;

    double fm_phase = NULL;

    int hackrf_sample = 2000000;

    if (mode == 0) {
        fm_deviation = 2.0 * M_PI * 75.0e3 / hackrf_sample; // 75 kHz max deviation WBFM
    }
    else if (mode == 1)
    {
        fm_deviation = 2.0 * M_PI * 5.0e3 / hackrf_sample; // 5 kHz max deviation NBFM
    }

    //AM mode
    if (mode == 2) {
        for (uint32_t i = 0; i < BUF_LEN; i++) {
            double	audio_amp = input[i] * gain;

            if (fabs(audio_amp) > 1.0) {
                audio_amp = (audio_amp > 0.0) ? 1.0 : -1.0;
            }

            output[i * BYTES_PER_SAMPLE] = (int8_t)(audio_amp*127.0);
            output[i * BYTES_PER_SAMPLE + 1] = 0;
        }
    }
        //FM mode
    else {

        for (uint32_t i = 0; i < BUF_LEN / 2; i++) {

            double	audio_amp = input[i] * gain;

            if (fabs(audio_amp) > 1.0) {
                audio_amp = (audio_amp > 0.0) ? 1.0 : -1.0;
            }
            fm_phase += fm_deviation * audio_amp;
            while (fm_phase > (float)(M_PI))
                fm_phase -= (float)(2.0 * M_PI);
            while (fm_phase < (float)(-M_PI))
                fm_phase += (float)(2.0 * M_PI);

            output[i * BYTES_PER_SAMPLE] = (int8_t)(sin(fm_phase)*127.0);
            output[i * BYTES_PER_SAMPLE + 1] =(int8_t)(cos(fm_phase)*127.0);
        }
    }


}

void Read_Wave(char * path){

    WaveData_t *wave = wavRead(path, strlen(path));
    int nch = wave->header.numChannels;
    _audioSampleRate=wave->sampleRate;
    numSampleCount = wave->size / wave->header.blockAlign;

    cout<<numSampleCount<<endl;
    _audioSampleBuf=new float[numSampleCount]();
    _new_audio_buf = new float[BUF_LEN/2]();

    if(nch==1){

        for(int i=0;i<numSampleCount;i++){

            _audioSampleBuf[i] = wave->samples[i];
        }

    }else if(nch==2){

            for(int i=0;i<numSampleCount;i++){

                _audioSampleBuf[i] = (wave->samples[i * 2] + wave->samples[i * 2 + 1]) / (float)2.0;

              //  cout<<"audio:"<<_audioSampleBuf[i]<<"sample:"<<wave->samples[i*2]<<endl;


        }
    }

}

void makeCache() {

    int nsample = (float)_audioSampleRate * (float)BUF_LEN / (float)_hackrfSampleRate / 2.0;

    int8_t ** iqCache = new int8_t*[numSampleCount / nsample]();
    for(int i = 0; i < numSampleCount / nsample; i++) {
        iqCache[i] = new int8_t[BUF_LEN]();
    }

    for(int i = 0; i < numSampleCount / nsample; i++) {
        interpolation(_audioSampleBuf + (nsample * i), nsample, _new_audio_buf, BUF_LEN / 2);
        modulation(_new_audio_buf, iqCache[i], 0);
    }

}


int hackrf_tx_callback(int8_t *buffer, uint32_t length) {
    int nsample = (float)_audioSampleRate * (float)length / (float)_hackrfSampleRate / 2.0;
    if(xcount <= (numSampleCount / nsample)) {
    memcpy(buffer, iqCache[xcount], length);
    return 1;
    }
    return 0;
}

int _hackrf_tx_callback(hackrf_transfer *transfer) {
    return hackrf_tx_callback((int8_t *)transfer->buffer, transfer->valid_length);
}

void hackrfWork() {
//    uint32_t hackrf_sample = 2000000;
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
