#include <hackrf.h>
#include <math.h>
#include <stdio.h>
#include <mutex>
#include "wavOperator.cpp"


#define BUF_LEN 262144         //hackrf tx buf
#define BUF_NUM  63
#define BYTES_PER_SAMPLE  2
#define M_PI 3.14159265358979323846

struct config {
    double freq;
    uint32_t gain;
    uint32_t mode;
    uint32_t tx_vga;
    uint32_t enableamp;

};

static config defaultSetting{
        433.00,//433Mhz
        90,
        0,//mode WBFM=0 NBFM=1 AM=2
        40,
        1
};


std::mutex m_mutex;
hackrf_device * _dev = NULL;
int ret;
uint64_t freq;
float gain;
uint32_t mode;
uint32_t tx_vga;
uint8_t enableamp;
bool inited = false;
uint32_t m_sample_rate;
size_t m_sample_count;
uint32_t nch;
uint32_t ch_mask;

config conf;
int8_t ** _buf = NULL;
int count;
int tail;
int head;
uint32_t hackrf_sample = 2000000;
float * audio_buf = NULL;
float * IQ_buf = NULL;
float * new_audio_buf = NULL;
double fm_phase = NULL;
double fm_deviation = NULL;
float last_in_samples[4] = { 0.0, 0.0, 0.0, 0.0 };

int _hackrf_tx_callback(hackrf_transfer *transfer);
int hackrf_tx_callback(int8_t *buffer, uint32_t length);
void interpolation(float * in_buf, uint32_t in_samples, float * out_buf, uint32_t out_samples, float last_in_samples[4]);
void modulation(float * input, float * output, uint32_t mode);
void work(float *input_items, uint32_t len);
bool on_chunk(audio_chunk * chunk, abort_callback &);



int hackrf_tx_callback(int8_t *buffer, uint32_t length) {

    m_mutex.lock();

    if (count == 0) {
        memset(buffer, 0, length);
    }
    else {
        memcpy(buffer, _buf[tail], length);
        tail = (tail + 1) % BUF_NUM;
        count--;
    }
    m_mutex.unlock();

    return 0;
}

int _hackrf_tx_callback(hackrf_transfer *transfer) {
    return hackrf_tx_callback((int8_t *)transfer->buffer, transfer->valid_length);
}


void interpolation(float * in_buf, uint32_t in_samples, float * out_buf, uint32_t out_samples, float last_in_samples[4]) {
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

            IQ_buf[i * BYTES_PER_SAMPLE] = (float)audio_amp;
            IQ_buf[i * BYTES_PER_SAMPLE + 1] = 0;
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
    int8_t * buf = (int8_t *)_buf[head];
    for (uint32_t i = 0; i < BUF_LEN; i++) {
        buf[i] = (int8_t)(input_items[i] * 127.0);
    }
    head = (head + 1) % BUF_NUM;
    count++;
    m_mutex.unlock();

}


bool on_chunk() {
    wavStruct wave = wavOperator();
    hackrf_sample = HACKRF_SAMPLE;
    uint16 nch = wave.header.numChannels;

    if (running) {

        if (!inited) {
            if (hackrf_sample > 20000000) {
                MessageBox(NULL, L"sample rate too high!", NULL, MB_OK);
            }
            hackrf_set_sample_rate(_dev, hackrf_sample);
            new_audio_buf = new float[BUF_LEN]();
            IQ_buf = new float[BUF_LEN * BYTES_PER_SAMPLE]();
            inited = true;
        }


        if (nch == 1) {
            for (uint32_t i = 0; i < m_sample_count; i++) {

                audio_buf[i] = wave.data[i];
            }
        }
        else if (nch == 2) {
            for (uint32_t i = 0; i < m_sample_count; i++) {

                audio_buf[i] = (wave.data[i * 2] + wave.data[i * 2 + 1]) / (float)2.0;
            }

        }




        interpolation(audio_buf, wave.subChunk2Size, new_audio_buf, BUF_LEN, last_in_samples);

        modulation(new_audio_buf, IQ_buf, 0);

        for (uint32_t i = 0; i < (BUF_LEN * BYTES_PER_SAMPLE); i += BUF_LEN) {

            work(IQ_buf + i, BUF_LEN);
        }

        //AM mode


    }
    // To retrieve the currently processed track, use get_cur_file().
    // Warning: the track is not always known - it's up to the calling component to provide this data and in some situations we'll be working with data that doesn't originate from an audio file.
    // If you rely on get_cur_file(), you should change need_track_change_mark() to return true to get accurate information when advancing between tracks.

    return true; //Return true to keep the chunk or false to drop it from the chain.
}

int main() {

    hackrf_init();
    ret = hackrf_open(&_dev);
    if (ret != HACKRF_SUCCESS) {
        MessageBox(NULL, L"Failed to open HackRF device", NULL, MB_OK);
        hackrf_close(_dev);
    }
    else {
        hackrf_set_sample_rate(_dev, hackrf_sample);
        hackrf_set_baseband_filter_bandwidth(_dev, 1750000);
        hackrf_set_freq(_dev, defaultSetting.freq);
        hackrf_set_txvga_gain(_dev, defaultSetting.tx_vga);
        hackrf_set_amp_enable(_dev, defaultSetting.enableamp);
        ret = hackrf_start_tx(_dev, _hackrf_tx_callback, (void *)this);
        if (ret != HACKRF_SUCCESS) {
            MessageBox(NULL, L"Failed to start TX streaming", NULL, MB_OK);
            hackrf_close(_dev);
        }
        running = true;
    }

}

