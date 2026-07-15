/*
* Copyright (c) 2026, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* *  Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* *  Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* *  Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Includes */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "buffer_manager.h"

/*Normalization for 14 bit raw audio data*/
#define SHIFT_BITS          (2U)
#define NORMALIZE_VALUE     (8192U)


/*Single audio sample from Analog or SPI microphone*/
volatile uint16_t audio_sample = 0;

/* 20 ms data (160 samples for 8 kHZ sampling rate
 * raw_audio_samples[] is filled inside the ADC ISR while
 * */
volatile int16_t raw_audio_samples[NUM_AUDIO_SAMPLES] = {0};
int16_t audio_sample_his[3] = {0,0};

/* Buffer Definitions */
/* raw_audio_samples_copy[] is processed in the feature extraction function */
volatile int16_t raw_audio_samples_copy[NUM_AUDIO_SAMPLES] = {0};

//__attribute__((retain))
//__attribute__((section(".input")))
///* Sliding window of the extracted feature. Input to the NN.
// * For TCDResetNet, the input size is 81*70. For TINIE, tow columns of zeros are padde
// * at the end for 32 bit memory alignment.
// * */

volatile int8_t audio_features [AUDIO_FEATURE_WINDOW * LPC_OUTPUT_SIZE] = {0};



/* Buffer Management Functions */

/* Implementation for Linear buffer
 * New Data is added at last index
 * Previous data is shifted one place up
 * */
void shift_NN_input_buffer(int16_t * extracted_feature)

{
    uint16_t i,j,k;

    //slide the window to the left by 1
    for(i = 0; i < (AUDIO_FEATURE_WINDOW-1); i++)
    {
        for(j = 0; j < LPC_OUTPUT_SIZE; j++)
        {
            audio_features[i*(LPC_OUTPUT_SIZE)+j] = audio_features[(i+1)*(LPC_OUTPUT_SIZE)+j];
        }
    }

    //update the last item in the window
    for(k = 0; k < LPC_OUTPUT_SIZE; k++)
    {
        audio_features[(i*(LPC_OUTPUT_SIZE)) + k] = (int8_t)extracted_feature[k];
    }

}

/* New data is added at the last index by the DMA every time new sample comes
 * shift all the data upwards to make room for the new data to be added
 * */
/*Audio Preprocessing for `LPC*/
void shift_audio_buffer(void)
{
    static uint16_t loop_count = 0;

    raw_audio_samples[loop_count] = (int16_t)((audio_sample -NORMALIZE_VALUE)  << SHIFT_BITS);

    if (loop_count == (NUM_AUDIO_SAMPLES - 1))
    {
        memcpy((uint8_t*)raw_audio_samples_copy,(uint8_t*)(raw_audio_samples),AUDIO_BUFFER_SIZE);
    }
    loop_count = (loop_count + 1) % NUM_AUDIO_SAMPLES;
}

