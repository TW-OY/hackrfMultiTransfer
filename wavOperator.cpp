#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#define HACKRF_SAMPLE 2000000
using namespace std;

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

struct wavHeaderStruct {
    uint8 chunkId[4];
    uint32 chunkSize;
    uint8 format[4];
    uint8 subChunk1ID[4];
    uint32 subChunk1Size;
    uint16 audioFormat;
    uint16 numChannels;
    uint32 sampleRate;
    uint32 byteRate;
    uint16 blockAlign;
    uint16 bitPerSample;
};

struct wavStruct {
    FILE *fp;
    wavHeaderStruct header;
    uint8 subChunk2ID[4];
    uint32 subChunk2Size;
    float *data;
};

wavStruct wavOperator() {
    wavStruct wave;
    wave.fp = fopen("/Users/MakeitBetter/agnew_nabobs.wav", "rb");


    if (wave.fp == NULL) {
        printf("error");
        return 0;
    }

    /* read head information */
    if (fread(wave.header.chunkId, sizeof(uint8), 4, wave.fp) != 4) {
        printf("read riff error!\n");
        return 0;
    }
    if (fread(&wave.header.chunkSize, sizeof(uint32), 1, wave.fp) == 0) {
        printf("read size error!\n");
        return 0;
    }
    if (fread(wave.header.format, sizeof(uint8), 4, wave.fp) != 4) {
        printf("read wave_flag error!\n");
        return 0;
    }
    if (fread(wave.header.subChunk1ID, sizeof(uint8), 4, wave.fp) != 4) {
        printf("read fmt error!\n");
        return 0;
    }
    if (fread(&wave.header.subChunk1Size, sizeof(uint32), 1, wave.fp) == 0) {
        printf("read fmt_len error!\n");
        return 0;
    }
    if (fread(&wave.header.audioFormat, sizeof(uint16), 1, wave.fp) == 0) {
        printf("read tag error!\n");
        return 0;
    }
    if (fread(&wave.header.numChannels, sizeof(uint16), 1, wave.fp) == 0) {
        printf("read channels error!\n");
        return 0;
    }
    if (fread(&wave.header.sampleRate, sizeof(uint32), 1, wave.fp) == 0) {
        printf("read samp_freq error!\n");
        return 0;
    }
    if (fread(&wave.header.byteRate, sizeof(uint32), 1, wave.fp) == 0) {
        printf("read byte_rate error!\n");
        return 0;
    }
    if (fread(&wave.header.blockAlign, sizeof(uint16), 1, wave.fp) == 0) {
        printf("read byte_samp error!\n");
        return 0;
    }
    if (fread(&wave.header.bitPerSample, sizeof(uint16), 1, wave.fp) == 0) {
        printf("read bit_samp error!\n");
        return 0;
    }

    if (fread(&wave.subChunk2ID, sizeof(uint8), 4, wave.fp) != 4) {
        printf("read subChunkID error!\n");
        return 0;
    }

    if (fread(&wave.subChunk2Size, sizeof(uint32), 1, wave.fp) == 0) {
        printf("read subChunkSize error!\n");
        return 0;
    }

    wave.data = new float[wave.subChunk2Size / 4]();
    fread(wave.data, sizeof(float), wave.subChunk2Size/4, wave.fp);

    return wave;
}

