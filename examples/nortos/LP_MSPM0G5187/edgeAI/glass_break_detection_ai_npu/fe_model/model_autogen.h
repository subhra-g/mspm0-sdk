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
#ifndef MODEL_AUTOGEN_H
#define MODEL_AUTOGEN_H

/* ============================================================
 * Full model geometry
 * ============================================================ */

#define MODEL_IN_CHANNELS            (1u)
#define MODEL_CONV_OUT_CHANNELS      (64u)
#define MODEL_CONV_KERNEL_T          (256u)
#define MODEL_CONV_STRIDE_T          (16u)
#define MODEL_MAX_OUT_CH_PER_ITER    (16u)
#define MODEL_FBANK_LEN              (MODEL_CONV_OUT_CHANNELS)
#define MODEL_WEIGHT_BITS            (2u)
#define MODEL_BRANCHED_BITS          (16u)
/* ============================================================
 * Execution slice geometry
 * ============================================================ */

#define MODEL_NUM_OUTPUT_BLOCKS      (4u)
#define MODEL_NUM_KERNEL_SLICES      (1u)
#define MODEL_PARAMS_LOAD_CAPACITY   (258u)
#define MODEL_ARBIAS0_ROW_WORD64     (128u)

/* CTL0 values */

#define MODEL_FE_CTL0_MSB (0x12019u)
#define MODEL_FE_CTL0_LSB (0x12011u)

#endif /* MODEL_AUTOGEN_H */
