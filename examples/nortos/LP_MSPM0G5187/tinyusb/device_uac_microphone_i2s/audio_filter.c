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

#include "audio_filter.h"

#define _IQ27(A) ((int32_t) ((A) * ((int32_t) 1 << 27)))

/* 300-3400Hz */
static const int32_t A0_Q27 = _IQ27(0.161872f);
static const int32_t A1_Q27 = _IQ27(0.0f);
static const int32_t A2_Q27 = _IQ27(-0.161872f);
static const int32_t B1_Q27 = _IQ27(-1.652339f);
static const int32_t B2_Q27 = _IQ27(0.676256f);

#define INPUT_DECAY_SHIFT 6
#define OUTPUT_DECAY_SHIFT 8

#define FRAME_ENERGY_SHIFT 8
#define ENERGY_BLOCK_NUM 16
#define NOISE_THRES_LIMIT 16384  // 1 Block

#define GAIN_DECAY_SHIFT 8
#define GAIN_ATTACK_SHIFT 4
#define MIN_NOISE_GAIN (3276 * 2)  // reserve some background noise

#define HANGOVER_BLOCKS 50  // per 16ms

void bandpass_filter_none_int32(
    int32_t* input_32bit, int32_t* out_32bit, uint16_t length)
{
    for (uint16_t cnt = 0; cnt < length; cnt++) {
        out_32bit[cnt] = input_32bit[cnt] >> OUTPUT_DECAY_SHIFT;
    }
    return;
}

void bandpass_filter_df1_int32(
    int32_t* input_32bit, int32_t* out_32bit, uint16_t length)
{
    static uint32_t dynamic_gain = 65536;

    static uint8_t is_silent         = 1; /* Track noise gate state */
    static uint8_t block_count       = 0;
    static uint16_t hangover_counter = HANGOVER_BLOCKS;
    static int32_t frame_energy =
        0;  // record the voice energy per frame - 48 points
    static int32_t block_energy = 0;

    static int32_t hpf_x1 = 0, hpf_y1 = 0;

    static int32_t x1 = 0, x2 = 0;
    static int32_t y1 = 0, y2 = 0;
    static int64_t noise_error = 0;

    for (uint16_t cnt = 0; cnt < length; cnt++) {
        // ==========================================
        // HPF - 15Hz
        // ==========================================
        int32_t raw_in = input_32bit[cnt];
        int32_t hpf_y0 = raw_in - hpf_x1 + hpf_y1 - (hpf_y1 >> 9);
        hpf_x1         = raw_in;
        hpf_y1         = hpf_y0;
        // ----------------------------------------------------
        // Input reduction
        // ----------------------------------------------------
        int32_t x0 = hpf_y0 >> INPUT_DECAY_SHIFT;
        // ----------------------------------------------------
        // Direct Form I Filter
        // ----------------------------------------------------
        int64_t y0_accum = ((int64_t) A0_Q27 * x0) + ((int64_t) A1_Q27 * x1) +
                           ((int64_t) A2_Q27 * x2) - ((int64_t) B1_Q27 * y1) -
                           ((int64_t) B2_Q27 * y2) + noise_error;
        int64_t y0 =
            (y0_accum + 67108864LL) >> 27;  // release IQ27 & Rounding 0.5
        noise_error =
            y0_accum - (y0 << 27);  // Record noise quantization error
        if (y0 > (int64_t) (2147483647 >> INPUT_DECAY_SHIFT))
            y0 = (int64_t) (2147483647 >> INPUT_DECAY_SHIFT);
        else if (y0 < (int64_t) (-2147483648 >> INPUT_DECAY_SHIFT))
            y0 = (int64_t) (-2147483648 >> INPUT_DECAY_SHIFT);
        // Update history value
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = (int32_t) y0;
        // ----------------------------------------------------
        // Sliding reduce background noise
        // ----------------------------------------------------
        int32_t filtered_sample =
            y1 << INPUT_DECAY_SHIFT;  // Process 32-bit for noise detection
        int32_t abs_val =
            (filtered_sample < 0) ? (-filtered_sample) : filtered_sample;
        int32_t temp = abs_val - frame_energy;
        frame_energy +=
            (temp + (1 << (FRAME_ENERGY_SHIFT - 1))) >> FRAME_ENERGY_SHIFT;
        block_energy += (frame_energy + 128) >> 8;
        /* Apply improved noise gate with hysteresis to prevent "flutter" */
        if (is_silent == 1) {
            if (dynamic_gain > MIN_NOISE_GAIN) { /* decrease gain */
                dynamic_gain -=
                    ((dynamic_gain - MIN_NOISE_GAIN) >> GAIN_DECAY_SHIFT) + 1;
                if (dynamic_gain < MIN_NOISE_GAIN)
                    dynamic_gain = MIN_NOISE_GAIN;
            }
        } else {
            if (dynamic_gain < 65536) { /* increase gain */
                dynamic_gain +=
                    ((65536 - dynamic_gain) >> GAIN_ATTACK_SHIFT) + 1;
                if (dynamic_gain > 65536) dynamic_gain = 65536;
            }
        }
        /* Apply gain for the filtered sample */
        int64_t sample_gain = ((int64_t) filtered_sample * dynamic_gain) >> 16;
        int32_t final_output = (int32_t) sample_gain;
        // ----------------------------------------------------
        // Output the filtered audio
        // ----------------------------------------------------
        out_32bit[cnt] = final_output >>
                         OUTPUT_DECAY_SHIFT;  //UAC is 16-bit, Input is 32-bit
    }
    // ----------------------------------------------------
    // Check the voice volume
    // ----------------------------------------------------
    block_count++;
    if (block_count >= ENERGY_BLOCK_NUM) {
        if (block_energy > NOISE_THRES_LIMIT) {
            is_silent = 0;
            hangover_counter =
                HANGOVER_BLOCKS;  // check the silent condition per x frame
        } else {
            if (hangover_counter > 0) {
                hangover_counter--;  //
            } else {
                is_silent = 1;  //
            }
        }
        block_count  = 0;
        block_energy = 0;
    }
    // ----------------------------------------------------
    // End of filter function
    // ----------------------------------------------------
}

/**
* @brief Writes (Push) a batch of audio sample data into the circular buffer
* @param rb: Pointer to the circular buffer structure
* @param data: The 16-bit audio data source to be written
* @param count: The number of samples to be written (e.g., 48)
* @return The number of samples actually successfully written
*/
uint32_t audio_ring_buf_push(
    audio_ring_buf_t* rb, const int32_t* data, uint32_t count)
{
    uint32_t free_space = audio_ring_buf_get_free(rb);

    if (count > free_space) {
        count = free_space;
    }

    if (count == 0) return 0;

    uint32_t head = rb->head & AUDIO_RING_BUF_MASK;

    uint32_t first_part = AUDIO_RING_BUF_SIZE - head;

    if (count <= first_part) {
        memcpy((void*) &rb->buffer[head], (const void*) data,
            count * sizeof(int32_t));
    } else {
        memcpy((void*) &rb->buffer[head], (const void*) data,
            first_part * sizeof(int32_t));
        memcpy((void*) &rb->buffer[0], (const void*) &data[first_part],
            (count - first_part) * sizeof(int32_t));
    }

    rb->head += count;

    return count;
}

/**
* @brief Extracts (Pops) a batch of audio sample data from the circular buffer.
* @param rb: Pointer to the circular buffer structure.
* @param dest: The target array to store the extracted data.
* @param count: The number of sample points to extract (e.g., 48).
* @return The actual number of sample points successfully extracted.
*/
uint32_t audio_ring_buf_pop(
    audio_ring_buf_t* rb, int32_t* dest, uint32_t count)
{
    if (count == 0) return 0;

    uint32_t available = audio_ring_buf_get_count(rb);
    if (count > available) {
        count = available;
    }

    uint32_t tail = rb->tail & AUDIO_RING_BUF_MASK;

    uint32_t first_part = AUDIO_RING_BUF_SIZE - tail;

    if (count <= first_part) {
        memcpy((void*) dest, (const void*) &rb->buffer[tail],
            count * sizeof(int32_t));
    } else {
        memcpy((void*) dest, (const void*) &rb->buffer[tail],
            first_part * sizeof(int32_t));
        memcpy((void*) &dest[first_part], (const void*) &rb->buffer[0],
            (count - first_part) * sizeof(int32_t));
    }

    rb->tail += count;

    return count;
}

/**
* @brief Writes (Push) a batch of audio sample data into the circular buffer (overwrites anything that overflows past the tail)
* @param rb: Pointer to the circular buffer structure
* @param data: The 16-bit audio data source to be written
* @param count: The number of samples to be written (e.g., 48)
* @return The number of samples actually successfully written
*/
uint32_t audio_ring_buf_push_overwrite(
    audio_ring_buf_t* rb, const int32_t* data, uint32_t count)
{
    if (count == 0) return 0;

    if (count > AUDIO_RING_BUF_SIZE) {
        data  = &data[count - AUDIO_RING_BUF_SIZE];
        count = AUDIO_RING_BUF_SIZE;
    }

    __disable_irq();
    uint32_t current_count = rb->head - rb->tail;
    if ((current_count + count) > AUDIO_RING_BUF_SIZE) {
        uint32_t overflow_bytes =
            (current_count + count) - AUDIO_RING_BUF_SIZE;
        rb->tail += overflow_bytes;
    }

    uint32_t head       = rb->head & AUDIO_RING_BUF_MASK;
    uint32_t first_part = AUDIO_RING_BUF_SIZE - head;

    if (count <= first_part) {
        memcpy((void*) &rb->buffer[head], (const void*) data,
            count * sizeof(int32_t));
    } else {
        memcpy((void*) &rb->buffer[head], (const void*) data,
            first_part * sizeof(int32_t));
        memcpy((void*) &rb->buffer[0], (const void*) &data[first_part],
            (count - first_part) * sizeof(int32_t));
    }

    rb->head += count;
    __enable_irq();

    return count;
}
