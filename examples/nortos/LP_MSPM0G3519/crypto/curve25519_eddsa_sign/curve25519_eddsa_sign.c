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
 *  @file       curve25519_eddsa_sign.c
 *  @brief      Edwards curve (Ed25519) digital signature generation example
 *
 *  This code example demonstrates how to use the Ed25519 sign function
 *  to generate a signature with Ed25519 public and private keys.
 *
 *  The example is based on Test Vector 2 from RFC-8032.
 *
 ******************************************************************************
 */

#include "ti_msp_dl_config.h"
#include "ti/crypto/include/msp-crypto-lib.h"

uint8_t __attribute__((aligned(4))) privateKey[ED25519_PRIVATE_KEY_LENGTH] = {
    0x4c, 0xcd, 0x08, 0x9b, 0x28, 0xff, 0x96, 0xda, 0x9d, 0xb6, 0xc3, 0x46,
    0xec, 0x11, 0x4e, 0x0f, 0x5b, 0x8a, 0x31, 0x9f, 0x35, 0xab, 0xa6, 0x24,
    0xda, 0x8c, 0xf6, 0xed, 0x4f, 0xb8, 0xa6, 0xfb,
};

uint8_t __attribute__((aligned(4))) publicKey[ED25519_PUBLIC_KEY_LENGTH] = {
    0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a, 0x92, 0xb7, 0x0a, 0xa7,
    0x4d, 0x1b, 0x7e, 0xbc, 0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4, 0x96, 0x8c,
    0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c,
};

uint8_t __attribute__((aligned(4))) expected_signature[ED25519_SIGNATURE_LENGTH] = {
    0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8, 0x72, 0x0e, 0x82, 0x0b,
    0x5f, 0x64, 0x25, 0x40, 0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50, 0x3f, 0x8f,
    0xb3, 0x76, 0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda, 0x08, 0x5a, 0xc1, 0xe4,
    0x3e, 0x15, 0x99, 0x6e, 0x45, 0x8f, 0x36, 0x13, 0xd0, 0xf1, 0x1d, 0x8c,
    0x38, 0x7b, 0x2e, 0xae, 0xb4, 0x30, 0x2a, 0xee, 0xb0, 0x0d, 0x29, 0x16,
    0x12, 0xbb, 0x0c, 0x00,
};

uint8_t __attribute__((aligned(4))) signature[ED25519_SIGNATURE_LENGTH];

uint8_t message[] = { 0x72 };
uint32_t message_len = 1;

uint8_t ED25519_signature_equal(const uint8_t sig1[ED25519_SIGNATURE_LENGTH],
                            const uint8_t sig2[ED25519_SIGNATURE_LENGTH]);

int main(void)
{
    crypto_ed25519_stat_t stat;

    SYSCFG_DL_init();

    /* Run the verify operation */
    stat = Crypto_ED25519_sign(
        message,
        message_len,
        publicKey,
        privateKey,
        signature
        );

    /* If the operation was not successful, jump to error trap */
    if (stat != CRYPTO_ED25519_STAT_SUCCESS ||
        !ED25519_signature_equal(signature, expected_signature)) {
        goto err;
    }

    /* Trap here on a pass condition */
    while (1) {
        __NOP();
    }

err:
    /* Trap here on an error condition */
    while (1) {
        __NOP();
    }
}

/*
 * Compare two ED25519 signatures for equality without using memcmp.
 * Returns true if the signatures are identical, false otherwise.
 */
uint8_t ED25519_signature_equal(const uint8_t sig1[ED25519_SIGNATURE_LENGTH],
                            const uint8_t sig2[ED25519_SIGNATURE_LENGTH]) {
    // Time-constant comparison to help prevent timing attacks
    uint8_t diff = 0;

    for (size_t i = 0; i < ED25519_SIGNATURE_LENGTH; i++) {
        diff |= sig1[i] ^ sig2[i];
    }

    return diff == 0;
}