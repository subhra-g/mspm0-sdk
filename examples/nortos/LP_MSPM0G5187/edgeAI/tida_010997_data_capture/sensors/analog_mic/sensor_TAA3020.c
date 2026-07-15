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
 * @file sensor_TAA3020.c
 * @brief TAA3020 2-Channel Audio ADC Driver
 *
 * Pin connections:
 *   IN1P / IN1M  <- ICS40740 differential output
 *   SDA / SCL    <- I2C bus
 *   BCLK         <- I2S bit clock
 *   FSYNC        <- I2S frame sync
 *   SDOUT        -> I2S data input
 *   AVDD / IOVDD <- 3.3V
 *   AREG         -> 10uF || 100nF cap to AVSS
 *   DREG         -> 10uF || 100nF cap to VSS
 *   VREF         -> 1uF cap to AVSS
 *   MICBIAS      -> 1uF cap to AVSS (if used for ICS40740 VDD)
 */

#include "sensor_TAA3020.h"
#include "i2c_comm.h"

/* Number of CPU cycles for 1ms delay */
#define CYCLES_PER_MS          (80000)

/* Macro to convert duration (ms) to hardware clock ticks */
#define MS_TO_CYCLES(ms)       ((ms) * (CYCLES_PER_MS))

/* -------------------------------------------------------------------------
 * Internal driver state
 * -------------------------------------------------------------------------*/
static struct {
    uint8_t initialised;
} s_drv;

/* -------------------------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------------------------*/

/**
 * @brief Write data to a TAA3020 register
 *
 * @param reg Register address
 * @param val Data to write
 * @return int 0 on success, error code otherwise
 */
static int taa3020_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;

    return I2C_write(I2C_TARGET_ADDR_TAA3020, buf, 2);
}

/**
 * @brief Read data from a TAA3020 register
 *
 * @param reg Register address
 * @param val Pointer to store the data
 * @return int 0 on success, error code otherwise
 */
static int taa3020_read_reg(uint8_t reg, uint8_t *val)
{
    int ret;

    /* Set register pointer */
    ret = I2C_write(I2C_TARGET_ADDR_TAA3020, &reg, 1);
    if (ret != TAA3020_OK) {
        return TAA3020_ERR_I2C;
    }

    /* Read register data */
    ret = I2C_read(I2C_TARGET_ADDR_TAA3020, val, 1);
    if (ret != TAA3020_OK) {
        return TAA3020_ERR_I2C;
    }

    return TAA3020_OK;
}

/**
 * @brief Select register page
 *
 * @param page Page number to select
 * @return int 0 on success, error code otherwise
 */
static int select_page(uint8_t page)
{
    return taa3020_write_reg(TAA3020_REG_PAGE_CFG, page);
}


/**
 * @brief Initialize the TAA3020 sensor
 *
 * @param cfg Pointer to configuration structure
 * @return int 0 on success, error code otherwise
 */
int taa3020_init(const taa3020_config_t *cfg)
{
    int ret;

    if (!cfg)
        return TAA3020_ERR_PARAM;

    s_drv.initialised = 0;

    /* Clamp fields to valid ranges */
    uint8_t wlen  = (cfg->word_length       > TAA3020_WL_32BIT)    ? TAA3020_WL_32BIT  : cfg->word_length;
    uint8_t imp   = (cfg->input_impedance   > TAA3020_IMP_20K)     ? TAA3020_IMP_20K   : cfg->input_impedance;
    uint8_t gain  = (cfg->pga_gain          > 84)                  ? 84                : cfg->pga_gain;
    uint8_t deci  = (cfg->decimation_filter > TAA3020_DECI_ULTRA_LOW) ? TAA3020_DECI_LINEAR : cfg->decimation_filter;
    uint8_t hpf   = (cfg->hpf_cutoff        > TAA3020_HPF_0_008_FS)  ? TAA3020_HPF_0_00025_FS : cfg->hpf_cutoff;
    uint8_t txOffset = (cfg->tx_offset      > 32)                  ? 32                : cfg->tx_offset;

    /* Step 1: Enable internal AREG and wake device */
    ret = select_page(0x00);
    if (ret != TAA3020_OK) return ret;

    ret = taa3020_write_reg(TAA3020_REG_SLEEP_CFG, TAA3020_SLEEP_AREG_INT | TAA3020_SLEEP_ENZ);
    if (ret != TAA3020_OK) return ret;

    /* Step 2: Software reset, then wait 100ms */
    ret = taa3020_write_reg(TAA3020_REG_SW_RESET, TAA3020_SW_RESET_BIT);
    if (ret != TAA3020_OK) return ret;
    delay_cycles(MS_TO_CYCLES(100));

    /* Step 3: Re-select page 0 and re-enable AREG after reset, then wait 10ms */
    ret = select_page(0x00);
    if (ret != TAA3020_OK) return ret;

    ret = taa3020_write_reg(TAA3020_REG_SLEEP_CFG, TAA3020_SLEEP_AREG_INT | TAA3020_SLEEP_ENZ);
    if (ret != TAA3020_OK) return ret;
    delay_cycles(MS_TO_CYCLES(20));

    /* Step 4: ASI format (I2S) and word length */
    ret = taa3020_write_reg(TAA3020_REG_ASI_CFG0,
                    (uint8_t)((TAA3020_ASI_FMT_I2S << 6) | (wlen << 4)));
    if (ret != TAA3020_OK) return ret;

    /* Step 5: Configure Tx Offset  */
    ret = taa3020_write_reg(TAA3020_REG_ASI_CFG1, txOffset);
    if (ret != TAA3020_OK) return ret;

    /* Step 6: CH1 input - differential, AC-coupled, configurable impedance
     *   D[6:5] = 00  : differential input
     *   D[4]   = 0   : AC-coupled
     *   D[3:2] = imp : input impedance */
    ret = taa3020_write_reg(TAA3020_REG_CH1_CFG0, (uint8_t)(imp << 2));
    if (ret != TAA3020_OK) return ret;

    /* Step 7: CH1 PGA gain - encoded as gain_steps in bits [7:1] */
    ret = taa3020_write_reg(TAA3020_REG_CH1_CFG1, (uint8_t)(gain << 1));
    if (ret != TAA3020_OK) return ret;

    /* Step 8: CH1 digital volume */
    ret = taa3020_write_reg(TAA3020_REG_CH1_CFG2, cfg->digital_volume);
    if (ret != TAA3020_OK) return ret;

    /* Step 9: DSP config - decimation filter mode D[5:4] and HPF cutoff D[1:0] */
    ret = taa3020_write_reg(TAA3020_REG_DSP_CFG0, (uint8_t)((deci << 4) | hpf));
    if (ret != TAA3020_OK) return ret;

    /* Step 10: Enable CH1 input path */
    ret = taa3020_write_reg(TAA3020_REG_IN_CH_EN, TAA3020_IN_CH1_EN);
    if (ret != TAA3020_OK) return ret;

    /* Step 11: Enable CH1 ASI output slot */
    ret = taa3020_write_reg(TAA3020_REG_ASI_OUT_CH_EN, TAA3020_ASI_OUT_CH1_EN);
    if (ret != TAA3020_OK) return ret;

    /* Step 12: Power up ADC. Add MICBIAS + PLL if required. */
    uint8_t pwr = TAA3020_PWR_ADC_UP;
    if (cfg->enable_micbias)
        pwr |= TAA3020_PWR_MICBIAS_UP | TAA3020_PWR_PLL_UP;

    ret = taa3020_write_reg(TAA3020_REG_PWR_CFG, pwr);
    if (ret != TAA3020_OK) return ret;
    delay_cycles(MS_TO_CYCLES(15));

    s_drv.initialised = 1;
    return TAA3020_OK;
}

/**
 * @brief Power down ADC and enter sleep mode
 *
 * @return int 0 on success, error code otherwise
 */
int taa3020_sleep(void)
{
    int ret;

    if (!s_drv.initialised)
        return TAA3020_ERR_NOT_INIT;

    ret = select_page(0x00);
    if (ret != TAA3020_OK) return ret;

    ret = taa3020_write_reg(TAA3020_REG_PWR_CFG, 0x00);
    if (ret != TAA3020_OK) return ret;

    /* Clear SLEEP_ENZ to enter sleep; keep AREG_SELECT so I2C stays accessible */
    ret = taa3020_write_reg(TAA3020_REG_SLEEP_CFG, TAA3020_SLEEP_AREG_INT);
    if (ret != TAA3020_OK) return ret;

    s_drv.initialised = 0;
    return TAA3020_OK;
}

/**
 * @brief Software reset the TAA3020 sensor
 *
 * @return int 0 on success, error code otherwise
 */
int taa3020_reset(void)
{
    int ret;

    s_drv.initialised = 0;
    (void)select_page(0x00);

    ret = taa3020_write_reg(TAA3020_REG_SW_RESET, TAA3020_SW_RESET_BIT);
    if (ret != TAA3020_OK) return ret;

    delay_cycles(MS_TO_CYCLES(100));
    return TAA3020_OK;
}

/**
 * @brief Write a byte to a Page-0 register
 *
 * @param reg Register address
 * @param value Value to write
 * @return int 0 on success, error code otherwise
 */
int taa3020_reg_write(uint8_t reg, uint8_t value)
{
    return taa3020_write_reg(reg, value);
}

/**
 * @brief Read a byte from a Page-0 register
 *
 * @param reg Register address
 * @param value Pointer to store the read value
 * @return int 0 on success, error code otherwise
 */
int taa3020_reg_read(uint8_t reg, uint8_t *value)
{
    return taa3020_read_reg(reg, value);
}
