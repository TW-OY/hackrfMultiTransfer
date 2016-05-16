#include "fmMod.h"
#include <math.h>
#include <mutex>
#include <iostream>
#include <libhackrf/hackrf.h>
#include "wavOperator.cpp"


std::mutex m_mutex;

namespace my {
    int head = 0;
    int count = 0;
    int tail = 0;
    int8_t **_buf = new int8_t* [BUF_NUM]();
}

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


void modulation(float * input, float * output, uint32_t mode) {
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

            output[i * BYTES_PER_SAMPLE] = (float)audio_amp;
            output[i * BYTES_PER_SAMPLE + 1] = 0;
        }
    }
        //FM mode
    else {

        for (uint32_t i = 0; i < BUF_LEN; i++) {

            double	audio_amp = input[i] * gain;

            if (fabs(audio_amp) > 1.0) {
                audio_amp = (audio_amp > 0.0) ? 1.0 : -1.0;
            }
            fm_phase += fm_deviation * audio_amp;
            while (fm_phase > (float)(M_PI))
                fm_phase -= (float)(2.0 * M_PI);
            while (fm_phase < (float)(-M_PI))
                fm_phase += (float)(2.0 * M_PI);

            output[i * BYTES_PER_SAMPLE] = (float)sin(fm_phase);
            output[i * BYTES_PER_SAMPLE + 1] = (float)cos(fm_phase);
        }
    }


}

void work(float *input_items, uint32_t len) {

    m_mutex.lock();
    int8_t * buf = my::_buf[my::head];
    for (uint32_t i = 0; i < BUF_LEN; i++) {
        buf[i] = (int8_t)(input_items[i] * 127.0);
    }
    my::head = (my::head + 1) % BUF_NUM;
    my::count++;
    m_mutex.unlock();

}


void on_chunk() {
    wavStruct wave = wavOperator();
    uint16 nch = wave.header.numChannels;
    uint32 numSampleCount = wave.subChunk2Size * 8 / wave.header.bitPerSample / wave.header.numChannels;

    if (my::_buf) {
        for (unsigned int i = 0; i < BUF_NUM; ++i) {
            my::_buf[i] = new int8_t[BUF_LEN]();
        }
    }

    float * audio_buf = new float[numSampleCount]();
    float * new_audio_buf = new float[BUF_LEN]();
    float * IQ_buf = new float[BUF_LEN * BYTES_PER_SAMPLE]();


    if (nch == 1) {
        for (uint32_t i = 0; i < numSampleCount; i++) {

            audio_buf[i] = wave.data[i];
        }
    }

    else if (nch == 2) {
        for (uint32_t i = 0; i < numSampleCount; i++) {

            audio_buf[i] = (wave.data[i * 2] + wave.data[i * 2 + 1]) / (float)2.0;
        }

    }

    interpolation(audio_buf, wave.subChunk2Size, new_audio_buf, BUF_LEN);

    modulation(new_audio_buf, IQ_buf, 0);

    for (uint32_t i = 0; i < (BUF_LEN * BYTES_PER_SAMPLE); i += BUF_LEN) {

        work(IQ_buf + i, BUF_LEN);
    }
//    delete audio_buf;
//    delete new_audio_buf;
//    delete IQ_buf;
}
    // To retrieve the currently processed track, use get_cur_file().
    // Warning: the track is not always known - it's up to the calling component to provide this data and in some situations we'll be working with data that doesn't originate from an audio file.
    // If you rely on get_cur_file(), you should change need_track_change_mark() to return true to get accurate information when advancing between tracks.

int hackrf_tx_callback(int8_t *buffer, uint32_t length) {

    m_mutex.lock();

    if (my::count == 0) {
        memset(buffer, 0, length);
    }
    else {
        memcpy(buffer, my::_buf[my::tail], length);
        my::tail = (my::tail + 1) % BUF_NUM;
        my::count--;
    }
    m_mutex.unlock();

    return 0;
}
void hackrfWork() {
    uint32_t hackrf_sample = 2000000;
    double freq = 433.00 * 1000000;
    uint32_t gain = 90 / 100.0;
    uint32_t tx_vga = 40;
    uint32_t enableamp = 1;
    hackrf_init();
    hackrf_device * _dev = NULL;
    int ret = hackrf_open(&_dev);
    cout<<ret<<endl;

//    if (ret != HACKRF_SUCCESS) {
//        hackrf_close(_dev);
//        cout<<"hackrf open error"<<endl;
//    }
//    else {
//        hackrf_set_sample_rate(_dev, hackrf_sample);
//        hackrf_set_baseband_filter_bandwidth(_dev, 1750000);
//        hackrf_set_freq(_dev, freq);
//        hackrf_set_txvga_gain(_dev, tx_vga);
//        hackrf_set_amp_enable(_dev, enableamp);
//        ret = hackrf_start_tx(_dev, hackrf_tx_callback(my::_buf, BUF_LEN), NULL);
//        if (ret != HACKRF_SUCCESS) {
//            hackrf_close(_dev);
//            cout<<"hackrf transfer error"<<endl;
//        }
//    }
}
