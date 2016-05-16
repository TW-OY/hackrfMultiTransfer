#ifndef HACKRFMULTITRANSFER_FMMOD_H
#define HACKRFMULTITRANSFER_FMMOD_H

#include <stdlib.h>

#define BUF_LEN 262144         //hackrf tx buf
#define BUF_NUM  63
#define BYTES_PER_SAMPLE  2
#define M_PI 3.14159265358979323846


void interpolation(float * in_buf, uint32_t in_samples, float * out_buf, uint32_t out_samples);
void modulation(float * input, float * output, uint32_t mode);
void work(float *input_items, uint32_t len);
void on_chunk();
int hackrf_tx_callback(int8_t *buffer, uint32_t length, int count);
void hackrfWork();

#endif //HACKRFMULTITRANSFER_WAVOPERATOR_H
