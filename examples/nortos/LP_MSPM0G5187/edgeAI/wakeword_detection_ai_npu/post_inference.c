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
#include "math.h"
#include "common.h"
#include "post_inference.h"

static const int8_t threshold = 0;
uint16_t acceptCounter = 0;
int16_t classZeroCounter = 0;

static uint8_t wwDetectCounter = 0;

uint16_t maxClass(float *input,uint16_t inputSize)
{
    float curMax = input[0];
    uint16_t maxIndex = 0;
    for (uint16_t i = 1; i < inputSize; i++)
    {
        if (curMax < input[i])
        {
            curMax = input[i];
            maxIndex = i;
        }
    }
    return maxIndex;
}



void post_inference_ww(int8_t * NN_output)
{
    if(NN_output[0] > NN_output[1])
    {
        wwDetectCounter++;

        if (wwDetectCounter >= 6)
        {
            classZeroCounter++;
            DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_LED_1_PIN ); // blue LED will glow up
        }
    }
    else
    {
        wwDetectCounter = 0;
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_LED_1_PIN );
    }

    return;
}
