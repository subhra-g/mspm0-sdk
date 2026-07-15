/*****************************************************************************/
/* Copyright (c) 2025 Texas Instruments Incorporated                         */
/* http://www.ti.com/                                                        */
/*                                                                           */
/*  Redistribution and  use in source  and binary forms, with  or without    */
/*  modification,  are permitted provided  that the  following conditions    */
/*  are met:                                                                 */
/*                                                                           */
/*     Redistributions  of source  code must  retain the  above copyright    */
/*     notice, this list of conditions and the following disclaimer.         */
/*                                                                           */
/*     Redistributions in binary form  must reproduce the above copyright    */
/*     notice, this  list of conditions  and the following  disclaimer in    */
/*     the  documentation  and/or   other  materials  provided  with  the    */
/*     distribution.                                                         */
/*                                                                           */
/*     Neither the  name of Texas Instruments Incorporated  nor the names    */
/*     of its  contributors may  be used to  endorse or  promote products    */
/*     derived  from   this  software  without   specific  prior  written    */
/*     permission.                                                           */
/*                                                                           */
/*  THIS SOFTWARE  IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS    */
/*  "AS IS"  AND ANY  EXPRESS OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT    */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR    */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT    */
/*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    */
/*  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT  NOT    */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,    */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    */
/*  THEORY OF  LIABILITY, WHETHER IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     */
/*                                                                           */
/*****************************************************************************/
#ifndef FE_MODEL_H
#define FE_MODEL_H

#include <stdint.h>
#include "model_autogen.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Filterbank parameter descriptor
 * ============================================================ */

/**
 * @brief Descriptor for one TINIE parameter image.
 *
 */
typedef struct
{
    const uint32_t *params;
    uint32_t words;
    uint16_t mmr0_offset_words;
} FB_ParamSlice;

/**
 * @brief Packed parameter image descriptors.
 *
 */
extern const FB_ParamSlice
g_fb_param_slices[MODEL_NUM_OUTPUT_BLOCKS][MODEL_NUM_KERNEL_SLICES];


/* ============================================================
 * TINIE instruction image
 * ============================================================ */

/**
 * @brief TINIE instruction memory image.
 *
 */
extern const uint32_t FB_INS[];
extern const uint32_t FB_INS_LEN;


/* ============================================================
 * TINIE MMR/register configuration image
 * ============================================================ */

/**
 * @brief TINIE MMR configuration image.
 *
 */
extern const uint32_t FBANK_MMR[];
extern const uint32_t FBANK_MMR_LEN;


/* ============================================================
 * Filterbank integer postprocess constants
 * ============================================================ */

/**
 * @brief Per-channel offset used after maxpool.
 *
 */
extern const int16_t FB_OFFSET[];
extern const uint32_t FB_OFFSET_LEN;

/**
 * @brief Per-channel scale used after maxpool.
 */
extern const uint8_t FB_SCALE[];
extern const uint32_t FB_SCALE_LEN;

/**
 * @brief Per-channel right-shift used after maxpool.
 */
extern const uint8_t FB_SHIFT[];
extern const uint32_t FB_SHIFT_LEN;


#ifdef __cplusplus
}
#endif

#endif /* FE_MODEL_H */