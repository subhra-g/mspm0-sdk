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

#include <stdio.h>
#include <limits.h>
#include "buffer_manager.h"
#include "common.h"
#include "feature_extraction.h"


/* Number of New features Extracted from raw Audio */
volatile uint8_t new_features_extracted = 0;

#include "lpc_float/lpc_defs.h"

void extract_features_lpc(void)
{
    int32_t temp;
    extern void get_lpc(int16_t *buffer);
    extern int16_t lpc[LPC_BANKS];

    // 160 samples = 20ms
    get_lpc((int16_t*)raw_audio_samples_copy);

    //256 is from the previous scale factor in get_lpc()
    for (uint16_t i = 0; i < LPC_BANKS; i++)
    {
        temp = lpc[i];

        /* offset of 1 */
       temp += (1*256);
       temp *= 157;
       /* shift nby 5,  1/0.03125 = 32 */
       temp >>= (5+8);
       lpc[i] = (int16_t)temp;
       if (lpc[i] >= 127) lpc[i] = 127;
       if (lpc[i] < -128) lpc[i] = -128;

    }

    /* Add features to NN input buffer */
    shift_NN_input_buffer(&lpc[0]);
}
