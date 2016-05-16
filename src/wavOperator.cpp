/*
 * wavread.c
 *
 * Copyright 2013 Sachin Mousli <samousli@csd.auth.gr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef WAV_READ
#define WAV_READ

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define DEBUG 1

/*
 * A basic implementation of the 16-bit pcm wave file header.
 * Links explaining the RIFF-WAVE standard:
 *		http://www.topherlee.com/software/pcm-tut-wavformat.html
 *		https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 */
typedef struct _WaveHeader_t
{
    char chunkID[4];  			// 1-4		"RIFF"
    int32_t chunkSize; 			// 5-8
    char format[4];				// 9-12		"WAVE"
    char subchunkID[4];			// 13-16	"fmt\0"
    int32_t subchunkSize;		// 17-20
    uint16_t audioFormat;		// 21-22 	PCM = 1
    uint16_t numChannels;		// 23-24
    int32_t sampleRate;			// 25-28
    int32_t bytesPerSecond;		// 29-32
    uint16_t blockAlign;		// 33-34
    uint16_t bitDepth;			// 35-36	16bit support only
    char dataID[4];				// 37-40	"data"
    int32_t dataSize;			// 41-44
} WaveHeader_t;

typedef struct _WaveData_t
{
    _WaveHeader_t header;
    int16_t *samples;
    int32_t size;
    int32_t sampleRate;
    uint16_t bitDepth;
} WaveData_t;

/*
 * Prototypes
 */
void printHeaderInfo(WaveHeader_t);
WaveData_t* wavRead(char[],size_t);
void dumpDataToFile(WaveData_t);



//int main(int argc, char* argv[])
//{
//    if (DEBUG)
//    {
//        char input[50];
//        if (argc == 1)
//            strcpy(input, "input.wav");
//        else
//            strncpy(input, argv[1], 50);
//
//        WaveData_t *data = wavRead(input,strlen(input));
//        if (data != NULL)
//            dumpDataToFile(*data);
//    }
//    return EXIT_SUCCESS;
//}

WaveData_t* wavRead(char fileName[],size_t fileNameSize)
{
    //
    if (fileName[fileNameSize] != '\0')
    {
        fprintf(stderr,"wavRead: Invalid string format.\n");
    } else
    {
        FILE* filePtr = fopen(fileName, "r");
        if (filePtr == NULL)
        {
            perror("Unable to open file");
        }
        else
        {
            // Read header.
            WaveHeader_t header;
            fread(&header, sizeof(header), 1, filePtr);

            if (DEBUG)
                printHeaderInfo(header);

            // Check if the file is of supported format.
            if (	strncmp(header.chunkID, 	"RIFF", 4)	||
                    strncmp(header.format, 		"WAVE", 4)	||
                    strncmp(header.subchunkID, "fmt" , 3)	||
                    strncmp(header.dataID, 		"data", 4) 	||
                    header.audioFormat != 1 				||
                    header.bitDepth != 16)
            {
                fprintf(stderr, "Unsupported file type.\n");
            }
            else
            {
                // Initialize the data struct.
                WaveData_t *data = (WaveData_t*) malloc(sizeof(WaveData_t));
                data->header = header;
                data->sampleRate = header.sampleRate;
                data->bitDepth	= header.bitDepth;
                data->size		= header.dataSize;

                // Read data.
                // ToDo: Add support for 24-32bit files.
                // 24bit samples are best converted to 32bits
                data->samples = (int16_t*) malloc(header.dataSize * sizeof(int16_t));
                fread(data->samples, sizeof(float), header.dataSize, filePtr);
                fclose(filePtr);
                return data;
            }
        }
    }
    return NULL;
}

void dumpDataToFile(WaveData_t waveData)
{
    // Dump data into a text file.
    FILE* outputFilePtr = fopen("output.txt","w");
    if (outputFilePtr == NULL)
    {
        perror("");
    }
    else
    {
        int i;
        for (i = 0; i < waveData.size; ++i)
        {
            fprintf(outputFilePtr,"%d\n",waveData.samples[i]);
        }
        fclose(outputFilePtr);
    }
}

/*
 * Prints the wave header
 */
void printHeaderInfo(WaveHeader_t hdr)
{
    char buf[5];
    printf("Header Info:\n");
    strncpy(buf, hdr.chunkID, 4);
    buf[4] = '\0';
    printf("	Chunk ID: %s\n",buf);
    printf("	Chunk Size: %d\n", hdr.chunkSize);
    strncpy(buf,hdr.format,4);
    buf[4] = '\0';
    printf("	Format: %s\n", buf);
    strncpy(buf,hdr.subchunkID,4);
    buf[4] = '\0';
    printf("	Sub-chunk ID: %s\n", buf);
    printf("	Sub-chunk Size: %d\n", hdr.subchunkSize);
    printf("	Audio Format: %d\n", hdr.audioFormat);
    printf("	Channel Count: %d\n", hdr.numChannels);
    printf("	Sample Rate: %d\n", hdr.sampleRate);
    printf("	Bytes per Second: %d\n", hdr.bytesPerSecond);
    printf("	Block alignment: %d\n", hdr.blockAlign);
    printf("	Bit depth: %d\n", hdr.bitDepth);
    strncpy(buf,hdr.dataID, 4);
    buf[4] = '\0';
    printf("	Data ID: %s\n", buf);
    printf("	Data Size: %d\n", hdr.dataSize);
}

#endif

