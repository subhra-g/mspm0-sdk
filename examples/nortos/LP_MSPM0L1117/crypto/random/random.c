 /*
 * Copyright (c) 2025, Texas Instruments Incorporated
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
/*!****************************************************************************
 *  @file       random.c
 *  @brief      Random number generation basic example
 * 
 *  This code example demonstrates how to use the rand entropy module to
 *  extract seed entropy, from which the rand drbg module may be seeded to
 *  enable generation of random numbers.
 * 
 *  The entropy moodule extracts 48 bytes of entropy from the TRNG.  Then,
 *  this data is used to seed a DRBG instance.  Then the example loops
 *  and outputs 32 bytes of random data per loop iteration.  Set a breakpoint
 *  on the __NOP in the main loop to observe the values changing in the
 *  randomData buffer.
 *
 ******************************************************************************
 */

#include "ti_msp_dl_config.h"
#include "ti/crypto/include/msp-crypto-lib.h"

/* Amount of entropy to collect in 32-bit words */
#define MCLK_FREQ (32000000U)

/* Amount of entropy to collect in 32-bit words */
#define ENTROPY_LEN (12U)

/* Amount of random data to collect from the DRBG, in 32-bit words */
#define RANDOM_LEN (8U)

/* Holding buffer for ENTROPY_LEN words of seed entropy from the TRNG */
static uint32_t entropy[ENTROPY_LEN];

/* The DRBG state data structure */
static crypto_drbg_ctx_t drbg;

/* Resulting random data from the seeded DRBG */
static uint32_t randomData[RANDOM_LEN];

int main(void)
{
    crypto_entropy_stat_t entropyStat;
    crypto_drbg_stat_t drbgStat;

    SYSCFG_DL_init();

    /* Get 32 bytes of seed entropy from the TRNG */
    entropyStat = Crypto_getEntropy(&entropy[0], ENTROPY_LEN, MCLK_FREQ);

    /* If the operation was not successful, jump to error trap */
    if (entropyStat != CRYPTO_ENTROPY_OK) {
        goto err;
    }

    /* Start the DRBG instance based on 8 words of entropy and 4 words 
     * of nonce, with both generated from the TRNG entropy source.
     */
    drbgStat = Crypto_DRBG_initialize(&drbg, &entropy[0], &entropy[8]);

    /* If the operation was not successful, jump to error trap */
    if (drbgStat != CRYPTO_DRBG_OK) {
        goto err;
    }

    /* Trap here on a pass condition */
    while (1) { 
        /* Generate random data block periodically from the DRBG */
        Crypto_DRBG_generate(&drbg, &randomData[0], RANDOM_LEN);

        /* If the operation was not successful, jump to error trap */
        if (drbgStat != CRYPTO_DRBG_OK) {
            goto err;
        }

        __NOP();
        delay_cycles(32000000); /* Delay 1s */
    }

err:
    /* Trap here on an error condition */
    while (1) { 
        __NOP(); 
    }
}
