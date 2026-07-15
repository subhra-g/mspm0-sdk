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

/**
 * @file sensor_TAA3020.h
 * @brief TAA3020 2-Channel Audio ADC Driver
 *
 * ICS40740 microphone connected to IN1P / IN1M (differential).
 * Control interface : I2C (7-bit address 0x4E, fixed)
 * Audio data output : I2S via SDOUT (BCLK / FSYNC, slave mode)
 */

#ifndef SENSOR_TAA3020_H
#define SENSOR_TAA3020_H

#include <stdint.h>
#include <stdbool.h>
#include <ti/driverlib/m0p/dl_core.h>

/* -------------------------------------------------------------------------
 * Error codes
 * -------------------------------------------------------------------------*/
#define TAA3020_OK                  0    /**< Success */
#define TAA3020_ERR_I2C            -1    /**< I2C communication error */
#define TAA3020_ERR_PARAM          -2    /**< Invalid parameter */
#define TAA3020_ERR_TIMEOUT        -3    /**< Timeout error */
#define TAA3020_ERR_I2S            -4    /**< I2S communication error */
#define TAA3020_ERR_NOT_INIT       -5    /**< Not initialized */

/* -------------------------------------------------------------------------
 * I2C address (7-bit, fixed)
 * -------------------------------------------------------------------------*/
#define I2C_TARGET_ADDR_TAA3020    0x4E   /* Binary 1001 110 */

/* -------------------------------------------------------------------------
 * Register addresses (Page 0)
 * -------------------------------------------------------------------------*/
#define TAA3020_REG_PAGE_CFG      0x00
#define TAA3020_REG_SW_RESET      0x01
#define TAA3020_REG_SLEEP_CFG     0x02
#define TAA3020_REG_ASI_CFG0      0x07
#define TAA3020_REG_ASI_CFG1      0x08
#define TAA3020_REG_MST_CFG0      0x13
#define TAA3020_REG_BIAS_CFG      0x3B
#define TAA3020_REG_CH1_CFG0      0x3C   /* Input source, impedance, coupling */
#define TAA3020_REG_CH1_CFG1      0x3D   /* PGA gain: bits [7:1] = gain_steps << 1 */
#define TAA3020_REG_CH1_CFG2      0x3E   /* Digital volume: 0=mute, 201=0dB, 255=27dB */
#define TAA3020_REG_DSP_CFG0      0x6B   /* Decimation filter mode, HPF, channel sum */
#define TAA3020_REG_IN_CH_EN      0x73   /* Input channel enable */
#define TAA3020_REG_ASI_OUT_CH_EN 0x74   /* ASI output channel enable */
#define TAA3020_REG_PWR_CFG       0x75   /* Power up: D7=MICBIAS, D6=ADC, D5=PLL */
#define TAA3020_REG_DEV_STS0      0x76   /* Channel power status (read-only) */
#define TAA3020_REG_DEV_STS1      0x77   /* Device mode status (read-only) */

/* -------------------------------------------------------------------------
 * Register bit-field constants
 * -------------------------------------------------------------------------*/

/* SLEEP_CFG (0x02) */
#define TAA3020_SLEEP_AREG_INT    0x80   /* D7=1: internal AREG (AVDD = 3.3V) */
#define TAA3020_SLEEP_ENZ         0x01   /* D0=1: device active (not sleeping) */

/* SW_RESET (0x01) */
#define TAA3020_SW_RESET_BIT      0x01   /* Self-clearing reset */

/* ASI_CFG0 (0x07): format field D[7:6] */
#define TAA3020_ASI_FMT_TDM       0x00
#define TAA3020_ASI_FMT_I2S       0x01
#define TAA3020_ASI_FMT_LJ        0x02

/* ASI_CFG0 (0x07): word length field D[5:4] */
#define TAA3020_WL_16BIT          0x00
#define TAA3020_WL_20BIT          0x01
#define TAA3020_WL_24BIT          0x02
#define TAA3020_WL_32BIT          0x03

/* CH1_CFG0 (0x3C): input impedance D[3:2] */
#define TAA3020_IMP_2K5           0x00   /* 2.5 kOhm (default) */
#define TAA3020_IMP_10K           0x01   /* 10 kOhm */
#define TAA3020_IMP_20K           0x02   /* 20 kOhm */

/* IN_CH_EN (0x73) */
#define TAA3020_IN_CH1_EN         0x80

/* ASI_OUT_CH_EN (0x74) */
#define TAA3020_ASI_OUT_CH1_EN    0x80

/* PWR_CFG (0x75) */
#define TAA3020_PWR_MICBIAS_UP    0x80   /* D7=1: power up MICBIAS */
#define TAA3020_PWR_ADC_UP        0x40   /* D6=1: power up ADC */
#define TAA3020_PWR_PLL_UP        0x20   /* D5=1: power up PLL */

/* DSP_CFG0 (0x6B): decimation filter mode D[5:4] */
#define TAA3020_DECI_LINEAR       0x00   /* Linear phase, default (~17 sample latency) */
#define TAA3020_DECI_LOW_LATENCY  0x01   /* Low latency (~7 sample latency) */
#define TAA3020_DECI_ULTRA_LOW    0x02   /* Ultra-low latency (~4 sample latency) */

/* DSP_CFG0 (0x6B): HPF cutoff D[1:0] */
#define TAA3020_HPF_0_00025_FS    0x01   /* 0.00025 x Fs, e.g. 12Hz @ 48kHz (default) */
#define TAA3020_HPF_0_002_FS      0x02   /* 0.002 x Fs,   e.g. 96Hz @ 48kHz */
#define TAA3020_HPF_0_008_FS      0x03   /* 0.008 x Fs,   e.g. 384Hz @ 48kHz */
#define TAA3020_HPF_DISABLED      0x00   /* Bypass HPF (flat response, no DC removal) */

/* -------------------------------------------------------------------------
 * Configuration structure
 * -------------------------------------------------------------------------*/
typedef struct {
    /* ASI */
    uint8_t word_length;        /* TAA3020_WL_16BIT / _20BIT / _24BIT / _32BIT */

    /* Channel 1 input */
    uint8_t input_impedance;    /* TAA3020_IMP_2K5 / _10K / _20K */

    /* Channel 1 gain */
    uint8_t pga_gain;           /* PGA gain: 0-84 steps (0-42 dB, 0.5 dB/step) */
    uint8_t digital_volume;     /* Digital volume: 0=mute, 201=0dB, 255=27dB */

    /* DSP */
    uint8_t decimation_filter;  /* TAA3020_DECI_LINEAR / _LOW_LATENCY / _ULTRA_LOW */
    uint8_t hpf_cutoff;         /* TAA3020_HPF_0_00025_FS / _0_002_FS / _0_008_FS / _DISABLED */

    /* ASI */
    uint8_t tx_offset;          /* Offset between data and BCLK cycle */

    /* Power */
    uint8_t enable_micbias;     /* 1 = power up MICBIAS output */
} taa3020_config_t;

/* -------------------------------------------------------------------------
 * Default configuration
 * Suitable as a starting point for ICS40740 at 48kHz I2S.
 * -------------------------------------------------------------------------*/
#define TAA3020_DEFAULT_CONFIG  {       \
    .word_length       = TAA3020_WL_32BIT,          \
    .input_impedance   = TAA3020_IMP_2K5,            \
    .pga_gain          = 0,    /* 0 dB */            \
    .digital_volume    = 201,  /* 0 dB */            \
    .decimation_filter = TAA3020_DECI_LINEAR,         \
    .hpf_cutoff        = TAA3020_HPF_0_00025_FS,     \
    .enable_micbias    = 0,                           \
}

/* -------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------*/

/**
 * @brief Initialise the TAA3020 and start ADC.
 *
 * Init sequence (verified from EVM script):
 *   1. SLEEP_CFG = 0x81  (enable internal AREG, wake device)
 *   2. SW_RESET  = 0x01  (full register reset, delay 100ms)
 *   3. PAGE_CFG  = 0x00  (re-select page 0 after reset)
 *   4. SLEEP_CFG = 0x81  (re-enable AREG, delay 10ms)
 *   5. ASI_CFG0          (I2S format + word length)
 *   6. CH1_CFG0          (differential input, AC-coupled, impedance)
 *   7. CH1_CFG1          (PGA gain)
 *   8. CH1_CFG2          (digital volume)
 *   9. DSP_CFG0          (decimation filter mode + HPF cutoff)
 *  10. IN_CH_EN  = 0x80  (enable CH1 input)
 *  11. ASI_OUT_CH_EN = 0x80  (enable CH1 ASI output slot)
 *  12. PWR_CFG          (power up ADC, optionally MICBIAS + PLL, delay 15ms)
 *
 * @param config Pointer to configuration structure
 * @return int 0 on success, error code otherwise
 */
int taa3020_init(const taa3020_config_t *config);

/**
 * @brief Power down ADC and enter sleep mode. Call taa3020_init() to resume.
 *
 * @return int 0 on success, error code otherwise
 */
int taa3020_sleep(void);

/**
 * @brief Software reset. All registers return to POR defaults.
 *
 * @return int 0 on success, error code otherwise
 */
int taa3020_reset(void);

/**
 * @brief Write a byte to a Page-0 register.
 *
 * @param reg Register address
 * @param value Value to write
 * @return int 0 on success, error code otherwise
 */
int taa3020_reg_write(uint8_t reg, uint8_t value);

/**
 * @brief Read a byte from a Page-0 register.
 *
 * @param reg Register address
 * @param value Pointer to store the read value
 * @return int 0 on success, error code otherwise
 */
int taa3020_reg_read(uint8_t reg, uint8_t *value);

#endif /* SENSOR_TAA3020_H */
