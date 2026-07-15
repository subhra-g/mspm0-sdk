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

#if 0
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#define __FPU_PRESENT (0U)
#include "arm_math.h"
#include <math.h>
#include <complex.h>
#include "lpc_constants.h"

//#define LPC_FLOAT_MATH

int16_t prev_buffer[WINDOW_OVERLAP];
int16_t data_8k[WINDOW_LEN];
int16_t data_prev;
int16_t y_preemp[WINDOW_LEN];
#ifdef LPC_FLOAT_MATH
float y_frames[WINDOW_LEN];
#else
int16_t y_frames[WINDOW_LEN];
#endif
float y_acorr[LPC_ORDER+1];
float acoeff[LPC_ORDER+1];
float err;
float kcoeff[LPC_ORDER];
float tmp[LPC_ORDER];
float mag[NUM_FREQ];
int16_t lpc[LPC_BANKS];

extern int debug;

#define DWT_CYCCNT_REG  0xF0001004

//log10f is exactly log2(x)/log2(10.0f)
#define log10f_fast(x)  (log2f_approx(x)*0.3010299956639812f)
float log2f_approx(float X) {
  float Y, F;
  int E;
  F = frexpf(fabsf(X), &E);
  Y = 1.23149591368684f;
  Y *= F;
  Y += -4.11852516267426f;
  Y *= F;
  Y += 6.02197014179219f;
  Y *= F;
  Y += -3.13396450166353f;
  Y += E;
  return(Y);
}

void levinson(float* in, uint16_t order, float* acoeff, float* err, float* kcoeff, float* tmp) {
    uint16_t i, j;
    float  acc;

    /* order 0 */
    acoeff[0] = (float)1.0;
    *err = in[0];

    /* order >= 1 */
    for (i = 1; i <= order; ++i) {
        acc = in[i];
        for ( j = 1; j <= i-1; ++j)
            acc += acoeff[j]*in[i-j];
        kcoeff[i-1] = -acc/(*err);
        acoeff[i] = kcoeff[i-1];
        for (j = 0; j < order; ++j)
            tmp[j] = acoeff[j];
        for (j = 1; j < i; ++j)
            acoeff[j] += kcoeff[i-1]*tmp[i-j];
        *err *= (1-kcoeff[i-1]*kcoeff[i-1]);
    }
}

//void print_zcoeff() {
//    uint16_t i, j;
//    complex float z, result;
//    uint16_t frqvec[NUM_FREQ] = {
//        112, 136, 161, 187, 212,
//        239, 266, 294, 322, 351,
//        380, 411, 442, 473, 505,
//        538, 572, 606, 641, 677,
//        714, 751, 790, 829, 869,
//        910, 951, 994, 1038, 1082,
//        1128, 1174, 1222, 1270, 1320,
//        1371, 1423, 1476, 1530, 1585,
//        1641, 1699, 1758, 1819, 1880,
//        1943, 2008, 2073, 2141, 2209,
//        2279, 2351, 2424, 2499, 2576,
//        2654, 2734, 2815, 2899, 2984,
//        3071, 3160, 3251, 3344, 3439,
//        3536, 3635, 3737, 3840, 3946
//    };
//    printf("complex float zcoeffs[NUM_FREQ][LPC_ORDER] = {\n");
//    for (i=0; i<NUM_FREQ; i++) {
//        z = cexp((complex)(2*I*M_PI*frqvec[i]/FS));
//        printf("    {");
//        for (j=1; j<=LPC_ORDER; j++) {
//            result = cpow(z,-j);
//            printf("%.10f + I*%.10f", creal(result), cimag(result));
//            if (j!=LPC_ORDER)
//                printf(", ");
//        }
//        printf("}");
//        if (i!=NUM_FREQ-1)
//            printf(",\n");
//    }
//    printf("};");
//}

void freqz_mag(float b, float *a) {
    uint16_t i, j;
    complex float num, deno, h;
    for (i=0; i<NUM_FREQ; i++) {
        //z = cexp((complex)(2*I*M_PI*frqvec[i]/FS));
        deno = 1.;
        for (j=1; j<=LPC_ORDER; j++)
            //deno += a[j]*cpow(z,-j);
            deno += a[j]*zcoeffs[i][j-1];
        num = b;
        //h = num*conj(deno)/pow(cabs(deno),2);
        h = num/deno;
        mag[i] = cabs(h*h);
    }
}

void freqz_mag_opt_cmsis(float b, float *a) {
    uint16_t i, j;
    float tmp[2], deno[2], deno_conj[2], h[2], h_sq[2];
    float deno_abs_sq, nume, h_sq_abs_sq;
    for (i=0; i<NUM_FREQ; i++) {
        deno[0] = 1.;
        deno[1] = 0.;
        for (j=1; j<=LPC_ORDER; j++) {
            arm_cmplx_mult_real_f32(&zcoeffs_opt[i][(j-1)<<1], &a[j], tmp, 1);
            deno[0] += tmp[0];
            deno[1] += tmp[1];
        }
        arm_cmplx_mag_squared_f32(deno, &deno_abs_sq, 1);
        arm_cmplx_conj_f32(deno, deno_conj, 1);
        nume = b/deno_abs_sq;
        arm_cmplx_mult_real_f32(deno_conj, &nume, h, 1);
        arm_cmplx_mult_cmplx_f32(h, h, h_sq, 1);
        arm_cmplx_mag_squared_f32(h_sq, &h_sq_abs_sq, 1);
        arm_sqrt_f32(h_sq_abs_sq, &mag[i]);
    }
}

void freqz_mag_opt(float b_sq, float *a) {
    uint16_t i, j;
    float deno[2], h[2], h_sq[2];
    float deno_abs_sq;
    for (i=0; i<NUM_FREQ; i++) {
        deno[0] = 1.;
        deno[1] = 0.;
        for (j=1; j<=LPC_ORDER; j++) {
            deno[0] += zcoeffs_opt[i][(j-1)<<1]*a[j];
            deno[1] += zcoeffs_opt[i][((j-1)<<1)+1]*a[j];
        }
        deno_abs_sq = deno[0]*deno[0] + deno[1]*deno[1];
        mag[i] = b_sq/deno_abs_sq;
    }
}
void my_arm_mult_q15(
  q15_t * pSrcA,
  q15_t * pSrcB,
  q15_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;                               /* loop counters */
  q31_t inA1, inA2, inB1, inB2;                  /* temporary input variables */
  q15_t out1, out2, out3, out4;                  /* temporary output variables */
  q31_t mul1, mul2, mul3, mul4;                  /* temporary variables */

  /* loop Unrolling */
  blkCnt = blockSize >> 2u;

  /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while(blkCnt > 0u)
  {
    /* read two samples at a time from sourceA */
    inA1 = *__SIMD32(pSrcA)++;
    /* read two samples at a time from sourceB */
    inB1 = *__SIMD32(pSrcB)++;
    /* read two samples at a time from sourceA */
    inA2 = *__SIMD32(pSrcA)++;
    /* read two samples at a time from sourceB */
    inB2 = *__SIMD32(pSrcB)++;

    /* multiply mul = sourceA * sourceB */
    mul1 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1 >> 16));
    mul2 = (q31_t) ((q15_t) inA1 * (q15_t) inB1);
    mul3 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) (inB2 >> 16));
    mul4 = (q31_t) ((q15_t) inA2 * (q15_t) inB2);

    /* Add 1u<<14 rounding */
    mul1 += 16384u;
    mul2 += 16384u;
    mul3 += 16384u;
    mul4 += 16384u;

    /* saturate result to 16 bit */
    out1 = (q15_t) __SSAT(mul1 >> 15, 16);
    out2 = (q15_t) __SSAT(mul2 >> 15, 16);
    out3 = (q15_t) __SSAT(mul3 >> 15, 16);
    out4 = (q15_t) __SSAT(mul4 >> 15, 16);

    /* store the result */
    *__SIMD32(pDst)++ = __PKHBT(out2, out1, 16);
    *__SIMD32(pDst)++ = __PKHBT(out4, out3, 16);

    /* Decrement the blockSize loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize % 0x4u;

  while(blkCnt > 0u)
  {
    /* C = A * B */
    /* Multiply the inputs and store the result in the destination buffer */
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> 15), 16);

    /* Decrement the blockSize loop counter */
    blkCnt--;
  }
}

void init_lpc() {
    memset((void *)&prev_buffer, 0, sizeof(prev_buffer));
    data_prev = 0;
}

void get_lpc(int16_t *buffer) {
    uint16_t i, j, idx;
    q63_t result;
    float acorr_max;
    float G, G_sqrt;
    float fb, fb_log;

    memmove(data_8k, prev_buffer, WINDOW_OVERLAP*sizeof(int16_t));
    memmove(data_8k+WINDOW_OVERLAP, buffer, FRONTEND_FRAME_LEN*sizeof(int16_t));
    memmove(prev_buffer, buffer+FRONTEND_FRAME_LEN-WINDOW_OVERLAP, WINDOW_OVERLAP*sizeof(int16_t));
    y_preemp[0] = data_8k[0]-data_prev;
    data_prev=*(buffer+FRONTEND_FRAME_LEN-WINDOW_OVERLAP-1);

    arm_sub_q15(data_8k+1, data_8k, y_preemp+1, WINDOW_LEN-1);

    //for (i=1; i<WINDOW_LEN; i++)
    //    y_preemp[i] = data_8k[i]-data_8k[i-1];

#ifdef LPC_FLOAT_MATH
    for (i=0; i<WINDOW_LEN; i++)
        y_frames[i] = y_preemp[i]*hamming_window[i];
#else
    arm_mult_q15(y_preemp, hamming_window_fx, y_frames, WINDOW_LEN);
    //my_arm_mult_q15(y_preemp, hamming_window_fx, y_frames, WINDOW_LEN);
#endif

    for (j=0; j<=LPC_ORDER; j++) {
#ifdef LPC_FLOAT_MATH
        arm_dot_prod_f32(y_frames, y_frames+j, WINDOW_LEN-j, y_acorr+j);
        //y_acorr[j] = 0.0;
        //for (i=0; i<WINDOW_LEN-j; i++)
        //    y_acorr[j] += y_frames[i+j]*y_frames[i];
#else
        arm_dot_prod_q15(y_frames, y_frames+j, WINDOW_LEN-j, &result);
        y_acorr[j] = (float)result;
        //GDN: convert result (q63_t) type to float
        //y_acorr[j] = i34_q30_to_float(result);
#endif
    }

    acorr_max = y_acorr[0]/LPC_ACORR_NOISE_SCALE; //Find the max. acorr value for lag=0
    if (acorr_max==0)
        acorr_max = 0.001;
    y_acorr[0] = 1.0;
    for (j=1; j<=LPC_ORDER; j++)
        y_acorr[j] /= acorr_max;

    levinson(y_acorr, LPC_ORDER, acoeff, &err, kcoeff, tmp);

    G = 0;
    for (j=0; j<=LPC_ORDER; j++)
    {
        G += acoeff[j]*y_acorr[j];
        printf("multipled %f with %f\n", acoeff[j], y_acorr[j]);
    }
    //G_sqrt = sqrt(G);
    //freqz_mag(G_sqrt, acoeff); // 600k cycles per 20ms slice
    freqz_mag_opt(G, acoeff);
    idx = 0;
    for (i=0; i<NUM_FREQ; i+=FRQ_PER_BANK) {
        fb = 0;
        for (j=i; j<i+FRQ_PER_BANK; j++)
            fb += mag[j];
        //fb_log = 10.*log10(fb/((float)FRQ_PER_BANK));
        lpc[idx++] = 2560.0*log10f_fast(fb/((float)FRQ_PER_BANK));
        //lpc[idx] = ((int16_t)((fb_log) * ((int32_t)1 << 8)));
        //idx++;
    }
}

#else
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#define __FPU_PRESENT (1U)
#include "arm_math.h"
#include <math.h>
#include <complex.h>
#include "lpc_constants.h"

//#define LPC_FLOAT_MATH

int16_t prev_buffer[WINDOW_OVERLAP] = {0};
int16_t data_8k[WINDOW_LEN] = {0};
int16_t data_prev = 0;
int16_t y_preemp[WINDOW_LEN] = {0};
#ifdef LPC_FLOAT_MATH
float y_frames[WINDOW_LEN]= {0};
#else
int16_t y_frames[WINDOW_LEN] = {0};
#endif
float y_acorr[LPC_ORDER+1] = {0};
float acoeff[LPC_ORDER+1] = {0};
float err = 0;
float kcoeff[LPC_ORDER] = {0};
float tmp[LPC_ORDER] = {0};
float mag[NUM_FREQ] = {0};
int16_t lpc[LPC_BANKS] = {0};

//log10f is exactly log2(x)/log2(10.0f)
#define log10f_fast(x)  (log2f_approx(x)*0.3010299956639812f)
float log2f_approx(float X) {
  float Y, F;
  int E;
  F = frexpf(fabsf(X), &E);
  Y = 1.23149591368684f;
  Y *= F;
  Y += -4.11852516267426f;
  Y *= F;
  Y += 6.02197014179219f;
  Y *= F;
  Y += -3.13396450166353f;
  Y += E;
  return(Y);
}

void levinson(float* in, uint16_t order, float* acoeff, float* err, float* kcoeff, float* tmp) {
    uint16_t i, j;
    float  acc;

    /* order 0 */
    acoeff[0] = (float)1.0;
    *err = in[0];

    /* order >= 1 */
    for (i = 1; i <= order; ++i) {
        acc = in[i];
        for ( j = 1; j <= i-1; ++j)
            acc += acoeff[j]*in[i-j];
        kcoeff[i-1] = -acc/(*err);
        acoeff[i] = kcoeff[i-1];
        for (j = 0; j < order; ++j)
            tmp[j] = acoeff[j];
        for (j = 1; j < i; ++j)
            acoeff[j] += kcoeff[i-1]*tmp[i-j];
        *err *= (1-kcoeff[i-1]*kcoeff[i-1]);
    }
}

//void print_zcoeff() {
//    uint16_t i, j;
//    complex float z, result;
//    uint16_t frqvec[NUM_FREQ] = {
//        112, 136, 161, 187, 212,
//        239, 266, 294, 322, 351,
//        380, 411, 442, 473, 505,
//        538, 572, 606, 641, 677,
//        714, 751, 790, 829, 869,
//        910, 951, 994, 1038, 1082,
//        1128, 1174, 1222, 1270, 1320,
//        1371, 1423, 1476, 1530, 1585,
//        1641, 1699, 1758, 1819, 1880,
//        1943, 2008, 2073, 2141, 2209,
//        2279, 2351, 2424, 2499, 2576,
//        2654, 2734, 2815, 2899, 2984,
//        3071, 3160, 3251, 3344, 3439,
//        3536, 3635, 3737, 3840, 3946
//    };
//    printf("complex float zcoeffs[NUM_FREQ][LPC_ORDER] = {\n");
//    for (i=0; i<NUM_FREQ; i++) {
//        z = cexp((complex)(2*I*M_PI*frqvec[i]/FS));
//        printf("    {");
//        for (j=1; j<=LPC_ORDER; j++) {
//            result = cpow(z,-j);
//            printf("%.10f + I*%.10f", creal(result), cimag(result));
//            if (j!=LPC_ORDER)
//                printf(", ");
//        }
//        printf("}");
//        if (i!=NUM_FREQ-1)
//            printf(",\n");
//    }
//    printf("};");
//}

void freqz_mag(float b, float *a) {
    uint16_t i, j;
    complex float num, deno, h;
    for (i=0; i<NUM_FREQ; i++) {
        //z = cexp((complex)(2*I*M_PI*frqvec[i]/FS));
        deno = 1.;
        for (j=1; j<=LPC_ORDER; j++)
            //deno += a[j]*cpow(z,-j);
            deno += a[j]*zcoeffs[i][j-1];
        num = b;
        //h = num*conj(deno)/pow(cabs(deno),2);
        h = num/deno;
        mag[i] = cabs(h*h);
    }
}

void freqz_mag_opt_cmsis(float b, float *a) {
    uint16_t i, j;
    float tmp[2], deno[2], deno_conj[2], h[2], h_sq[2];
    float deno_abs_sq, nume, h_sq_abs_sq;
    for (i=0; i<NUM_FREQ; i++) {
        deno[0] = 1.;
        deno[1] = 0.;
        for (j=1; j<=LPC_ORDER; j++) {
            arm_cmplx_mult_real_f32(&zcoeffs_opt[i][(j-1)<<1], &a[j], tmp, 1);
            deno[0] += tmp[0];
            deno[1] += tmp[1];
        }
        arm_cmplx_mag_squared_f32(deno, &deno_abs_sq, 1);
        arm_cmplx_conj_f32(deno, deno_conj, 1);
        nume = b/deno_abs_sq;
        arm_cmplx_mult_real_f32(deno_conj, &nume, h, 1);
        arm_cmplx_mult_cmplx_f32(h, h, h_sq, 1);
        arm_cmplx_mag_squared_f32(h_sq, &h_sq_abs_sq, 1);
        arm_sqrt_f32(h_sq_abs_sq, &mag[i]);
    }
}

void freqz_mag_opt(float b_sq, float *a) {
    uint16_t i, j;
    float deno[2], h[2], h_sq[2];
    float deno_abs_sq;
    for (i=0; i<NUM_FREQ; i++) {
        deno[0] = 1.;
        deno[1] = 0.;
        for (j=1; j<=LPC_ORDER; j++) {
            deno[0] += zcoeffs_opt[i][(j-1)<<1]*a[j];
            deno[1] += zcoeffs_opt[i][((j-1)<<1)+1]*a[j];
        }
        deno_abs_sq = deno[0]*deno[0] + deno[1]*deno[1];
        mag[i] = b_sq/deno_abs_sq;
    }
}
void my_arm_mult_q15(
  q15_t * pSrcA,
  const q15_t * pSrcB,
  q15_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;                               /* loop counters */
  q31_t inA1, inA2, inB1, inB2;                  /* temporary input variables */
  q15_t out1, out2, out3, out4;                  /* temporary output variables */
  q31_t mul1, mul2, mul3, mul4;                  /* temporary variables */

  /* loop Unrolling */
  blkCnt = blockSize >> 2u;

  /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.        
   ** a second loop below computes the remaining 1 to 3 samples. */
  while(blkCnt > 0u)
  {
    /* read two samples at a time from sourceA */
    inA1 = *__SIMD32(pSrcA)++;
    /* read two samples at a time from sourceB */
    inB1 = *__SIMD32(pSrcB)++;
    /* read two samples at a time from sourceA */
    inA2 = *__SIMD32(pSrcA)++;
    /* read two samples at a time from sourceB */
    inB2 = *__SIMD32(pSrcB)++;

    /* multiply mul = sourceA * sourceB */
    mul1 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1 >> 16));
    mul2 = (q31_t) ((q15_t) inA1 * (q15_t) inB1);
    mul3 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) (inB2 >> 16));
    mul4 = (q31_t) ((q15_t) inA2 * (q15_t) inB2);

    /* Add 1u<<14 rounding */
    mul1 += 16384u;
    mul2 += 16384u;
    mul3 += 16384u;
    mul4 += 16384u;

    /* saturate result to 16 bit */
    out1 = (q15_t) __SSAT(mul1 >> 15, 16);
    out2 = (q15_t) __SSAT(mul2 >> 15, 16);
    out3 = (q15_t) __SSAT(mul3 >> 15, 16);
    out4 = (q15_t) __SSAT(mul4 >> 15, 16);

    /* store the result */
    *__SIMD32(pDst)++ = __PKHBT(out2, out1, 16);
    *__SIMD32(pDst)++ = __PKHBT(out4, out3, 16);

    /* Decrement the blockSize loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.    
   ** No loop unrolling is used. */
  blkCnt = blockSize % 0x4u;

  while(blkCnt > 0u)
  {
    /* C = A * B */
    /* Multiply the inputs and store the result in the destination buffer */
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> 15), 16);

    /* Decrement the blockSize loop counter */
    blkCnt--;
  }
}

void init_lpc() {
    memset((void *)&prev_buffer, 0, sizeof(prev_buffer));
    data_prev = 0;
}

void get_lpc(int16_t *buffer) {
    uint16_t i, j, idx;
    q63_t result;
    float acorr_max;
    float G, G_sqrt;
    float fb, fb_log;

    memmove(data_8k, prev_buffer, WINDOW_OVERLAP*sizeof(int16_t));
    memmove(data_8k+WINDOW_OVERLAP, buffer, FRONTEND_FRAME_LEN*sizeof(int16_t));
    memmove(prev_buffer, buffer+FRONTEND_FRAME_LEN-WINDOW_OVERLAP, WINDOW_OVERLAP*sizeof(int16_t));
    y_preemp[0] = data_8k[0]-data_prev;
    data_prev=*(buffer+FRONTEND_FRAME_LEN-WINDOW_OVERLAP-1);

    arm_sub_q15(data_8k+1, data_8k, y_preemp+1, WINDOW_LEN-1);
    //for (i=1; i<WINDOW_LEN; i++)
    //    y_preemp[i] = data_8k[i]-data_8k[i-1];

#ifdef LPC_FLOAT_MATH    
    for (i=0; i<WINDOW_LEN; i++)
        y_frames[i] = y_preemp[i]*hamming_window[i];
#else
    //arm_mult_q15(y_preemp, hamming_window_fx, y_frames, WINDOW_LEN);
    my_arm_mult_q15(y_preemp, hamming_window_fx, y_frames, WINDOW_LEN);
#endif
 
    for (j=0; j<=LPC_ORDER; j++) {
#ifdef LPC_FLOAT_MATH
        arm_dot_prod_f32(y_frames, y_frames+j, WINDOW_LEN-j, y_acorr+j);
        //y_acorr[j] = 0.0;
        //for (i=0; i<WINDOW_LEN-j; i++)
        //    y_acorr[j] += y_frames[i+j]*y_frames[i];
#else
        arm_dot_prod_q15(y_frames, y_frames+j, WINDOW_LEN-j, &result);
        y_acorr[j] = (float)result;
#endif
    }
    
    acorr_max = y_acorr[0]/LPC_ACORR_NOISE_SCALE; //Find the max. acorr value for lag=0
    if (acorr_max==0)
        acorr_max = 0.001;
    y_acorr[0] = 1.0;
    for (j=1; j<=LPC_ORDER; j++)
        y_acorr[j] /= acorr_max;
    levinson(y_acorr, LPC_ORDER, acoeff, &err, kcoeff, tmp);
    G = 0;
    for (j=0; j<=LPC_ORDER; j++)
        G += acoeff[j]*y_acorr[j];
    //G_sqrt = sqrt(G);
    //freqz_mag(G_sqrt, acoeff); // 600k cycles per 20ms slice
    freqz_mag_opt(G, acoeff);
    idx = 0;
    for (i=0; i<NUM_FREQ; i+=FRQ_PER_BANK) {
        fb = 0;
        for (j=i; j<i+FRQ_PER_BANK; j++)
            fb += mag[j];
        //fb_log = 10.*log10(fb/((float)FRQ_PER_BANK));
        fb_log = 10.*log10f_fast(fb/((float)FRQ_PER_BANK));
        lpc[idx++] = ((int16_t)((fb_log) * ((int32_t)1 << 8)));
    }
}
#endif
