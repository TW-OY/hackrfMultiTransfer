#include "fmMod.h"
#include <math.h>

float _last_in_samples[4] = { 0.0, 0.0, 0.0, 0.0 };

void interpolation(float * in_buf, uint32_t in_samples, float * out_buf, uint32_t out_samples) {

    uint32_t i;		/* Input buffer index + 1. */
    uint32_t j = 0;	/* Output buffer index. */
    float pos;		/* Position relative to the input buffer
						* + 1.0. */

    /* We always "stay one sample behind", so what would be our first sample
    * should be the last one wrote by the previous call. */
    pos = (float)in_samples / (float)out_samples;
    while (pos < 1.0)
    {
        out_buf[j] = _last_in_samples[3] + (in_buf[0] - _last_in_samples[3]) * pos;
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

    /* Copy last samples to _last_in_samples (reusing i and j). */
    for (i = in_samples - 4, j = 0; j < 4; i++, j++)
        _last_in_samples[j] = in_buf[i];
}


void modulation(float * input, int8_t * output, uint32_t mode) {
    double fm_deviation = 0.0;
    float gain = 0.9;

    double fm_phase = 0.0;

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
