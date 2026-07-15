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

#ifndef AUDIO_FILTER_H_
#define AUDIO_FILTER_H_

#include <string.h>
#include "ti_msp_dl_config.h"

#define AUDIO_RING_BUF_SIZE 256 /* 5 frames, each being 48 */
#define AUDIO_RING_BUF_MASK (AUDIO_RING_BUF_SIZE - 1)

typedef struct {
    int32_t buffer[AUDIO_RING_BUF_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} audio_ring_buf_t;

static inline void audio_ring_buf_init(audio_ring_buf_t* rb)
{
    rb->head = 0;
    rb->tail = 0;
}

static inline uint32_t audio_ring_buf_get_count(const audio_ring_buf_t* rb)
{
    return (rb->head - rb->tail);
}

static inline uint32_t audio_ring_buf_get_free(const audio_ring_buf_t* rb)
{
    return (AUDIO_RING_BUF_SIZE - audio_ring_buf_get_count(rb));
}

static inline void convert_to_16bit_standard(
    const int32_t* src, int16_t* dest, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        int32_t sample = src[i];
        if (sample > 32767) {
            sample = 32767;
        } else if (sample < -32768) {
            sample = -32768;
        }
        dest[i] = (int16_t) sample;
    }
}
extern uint32_t audio_ring_buf_push(
    audio_ring_buf_t* rb, const int32_t* data, uint32_t count);

extern uint32_t audio_ring_buf_pop(
    audio_ring_buf_t* rb, int32_t* dest, uint32_t count);

extern uint32_t audio_ring_buf_push_overwrite(
    audio_ring_buf_t* rb, const int32_t* data, uint32_t count);

extern void bandpass_filter_df1_int32(
    int32_t* input_32bit, int32_t* out_32bit, uint16_t length);

extern void bandpass_filter_dft1_float(
    int32_t* input_32bit, int32_t* out_32bit, uint16_t length);
extern void bandpass_filter_none_int32(
    int32_t* input_32bit, int32_t* out_32bit, uint16_t length);
#endif /* AUDIO_FILTER_H_ */
