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

#include "sensor_BMI270.h"
#include "spi_comm.h"
#include <string.h>

/* Number of CPU cycles for 1ms delay */
#define CYCLES_PER_MS          (80000)

/* Macro to convert duration (ms) to hardware clock ticks */
#define MS_TO_CYCLES(ms)       ((ms) * (CYCLES_PER_MS))

/* SPI read/write bit */
#define BMI270_SPI_READ        0x80


/* Private function prototypes */
static int bmi270_read_reg(uint8_t regAddr, uint8_t *data, uint16_t len);
static int bmi270_write_reg(uint8_t regAddr, const uint8_t *data, uint16_t len);
static int bmi270_write_command(uint8_t command);
static void bmi270_switch_to_spi(void);

/* Global SPI Chip Select pin */
const chipSelect gBMI270_SPI_CS = {CS_PORT, CS_IMU_PIN};

/* BMI270 initialization data */
static const uint8_t bmi270_init_data[328] = {
    0xc8, 0x2e, 0x00, 0x2e, 0x80, 0x2e, 0x1a, 0x00, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, 0x00,
    0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0x90, 0x32, 0x21, 0x2e, 0x59, 0xf5,
    0x10, 0x30, 0x21, 0x2e, 0x6a, 0xf5, 0x1a, 0x24, 0x22, 0x00, 0x80, 0x2e, 0x3b, 0x00, 0xc8, 0x2e, 0x44, 0x47, 0x22,
    0x00, 0x37, 0x00, 0xa4, 0x00, 0xff, 0x0f, 0xd1, 0x00, 0x07, 0xad, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1,
    0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00,
    0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x24, 0xfc, 0xf5, 0x80, 0x30, 0x40, 0x42, 0x50, 0x50, 0x00, 0x30, 0x12, 0x24, 0xeb,
    0x00, 0x03, 0x30, 0x00, 0x2e, 0xc1, 0x86, 0x5a, 0x0e, 0xfb, 0x2f, 0x21, 0x2e, 0xfc, 0xf5, 0x13, 0x24, 0x63, 0xf5,
    0xe0, 0x3c, 0x48, 0x00, 0x22, 0x30, 0xf7, 0x80, 0xc2, 0x42, 0xe1, 0x7f, 0x3a, 0x25, 0xfc, 0x86, 0xf0, 0x7f, 0x41,
    0x33, 0x98, 0x2e, 0xc2, 0xc4, 0xd6, 0x6f, 0xf1, 0x30, 0xf1, 0x08, 0xc4, 0x6f, 0x11, 0x24, 0xff, 0x03, 0x12, 0x24,
    0x00, 0xfc, 0x61, 0x09, 0xa2, 0x08, 0x36, 0xbe, 0x2a, 0xb9, 0x13, 0x24, 0x38, 0x00, 0x64, 0xbb, 0xd1, 0xbe, 0x94,
    0x0a, 0x71, 0x08, 0xd5, 0x42, 0x21, 0xbd, 0x91, 0xbc, 0xd2, 0x42, 0xc1, 0x42, 0x00, 0xb2, 0xfe, 0x82, 0x05, 0x2f,
    0x50, 0x30, 0x21, 0x2e, 0x21, 0xf2, 0x00, 0x2e, 0x00, 0x2e, 0xd0, 0x2e, 0xf0, 0x6f, 0x02, 0x30, 0x02, 0x42, 0x20,
    0x26, 0xe0, 0x6f, 0x02, 0x31, 0x03, 0x40, 0x9a, 0x0a, 0x02, 0x42, 0xf0, 0x37, 0x05, 0x2e, 0x5e, 0xf7, 0x10, 0x08,
    0x12, 0x24, 0x1e, 0xf2, 0x80, 0x42, 0x83, 0x84, 0xf1, 0x7f, 0x0a, 0x25, 0x13, 0x30, 0x83, 0x42, 0x3b, 0x82, 0xf0,
    0x6f, 0x00, 0x2e, 0x00, 0x2e, 0xd0, 0x2e, 0x12, 0x40, 0x52, 0x42, 0x00, 0x2e, 0x12, 0x40, 0x52, 0x42, 0x3e, 0x84,
    0x00, 0x40, 0x40, 0x42, 0x7e, 0x82, 0xe1, 0x7f, 0xf2, 0x7f, 0x98, 0x2e, 0x6a, 0xd6, 0x21, 0x30, 0x23, 0x2e, 0x61,
    0xf5, 0xeb, 0x2c, 0xe1, 0x6f
};

/* Current configuration */
static bmi270_config_t gCurrentConfig;

/* Sensor initialization status */
static bool sensor_initialized __attribute__((aligned(4))) = false;

/**
 * @brief Initialize the BMI270 sensor
 *
 * @param config Pointer to configuration structure
 * @return int 0 on success, error code otherwise
 */
int bmi270_init(const bmi270_config_t *config)
{
    int ret;
    uint8_t chip_id;
    uint8_t status;

    /* Store configuration */
    memcpy(&gCurrentConfig, config, sizeof(bmi270_config_t));

    /* Switch to SPI mode */
    bmi270_switch_to_spi();

    /* Wait for device to switch to SPI mode */
    delay_cycles(MS_TO_CYCLES(10));

    /* Reset the sensor */
    ret = bmi270_reset();
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Wait for reset to complete */
    delay_cycles(MS_TO_CYCLES(10));

    /* Switch to SPI mode */
    bmi270_switch_to_spi();

    /* Wait for device to switch to SPI mode */
    delay_cycles(MS_TO_CYCLES(10));

    /* Check chip ID */
    ret = bmi270_read_reg(BMI270_REG_CHIP_ID, &chip_id, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    if (chip_id != BMI270_CHIP_ID) {
        return BMI270_ERR_INVALID_ID;
    }

    /* Disable advanced power save mode */
    uint8_t pwr_conf = 0x00;  /* Disable advanced power save */
    ret = bmi270_write_reg(BMI270_REG_PWR_CONF, &pwr_conf, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Wait for 450ms */
    delay_cycles(MS_TO_CYCLES(450));

    /* Prepare for initialization */
    uint8_t init_ctrl = 0x00;
    ret = bmi270_write_reg(BMI270_REG_INIT_CTRL, &init_ctrl, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Wait for 10ms */
    delay_cycles(MS_TO_CYCLES(10));

    /* Upload initialization data (Size = 328 bytes) */
    ret = bmi270_write_reg(BMI270_REG_INIT_DATA, &bmi270_init_data[0], sizeof(bmi270_init_data));

    if (ret != BMI270_OK) {
        return ret;
    }

    /* Complete initialization */
    init_ctrl = 0x01;
    ret = bmi270_write_reg(BMI270_REG_INIT_CTRL, &init_ctrl, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Wait for 100ms */
    delay_cycles(MS_TO_CYCLES(100));

    /* Wait for initialization to complete (max 20ms) */
    int timeout = 20;
    while(--timeout)
    {
        delay_cycles(MS_TO_CYCLES(1));
        ret = bmi270_read_reg(0x21, &status, 1);  /* INTERNAL_STATUS register */
        if (ret != BMI270_OK) {
            return ret;
        }

        if (status & 0x01) {  /* Check if init_ok bit is set */
            break;
        }
    }

    if (timeout == 0)
    {
        return BMI270_ERR_TIMEOUT;
    }

    /* Configure accelerometer if enabled */
    if (config->sensorEnable & BMI270_SENSOR_ACC)
    {
        uint8_t acc_conf = (config->accBwp << 4) | config->accOdr;
        if (config->accFilterPerf) {
            acc_conf |= 0x80;  /* Set acc_filter_perf bit */
        }
        ret = bmi270_write_reg(BMI270_REG_ACC_CONF, &acc_conf, 1);
        if (ret != BMI270_OK) {
            return ret;
        }

        /* Configure accelerometer range */
        uint8_t acc_range = config->accRange;
        ret = bmi270_write_reg(BMI270_REG_ACC_RANGE, &acc_range, 1);
        if (ret != BMI270_OK) {
            return ret;
        }
    }

    /* Configure gyroscope if enabled */
    if (config->sensorEnable & BMI270_SENSOR_GYR) {
        uint8_t gyr_conf = (config->gyrBwp << 4) | config->gyrOdr;
        if (config->gyrFilterPerf) {
            gyr_conf |= 0x80;  /* Set gyr_filter_perf bit */
        }
        if (config->gyrNoisePerf) {
            gyr_conf |= 0x40;  /* Set gyr_noise_perf bit */
        }
        ret = bmi270_write_reg(BMI270_REG_GYR_CONF, &gyr_conf, 1);
        if (ret != BMI270_OK) {
            return ret;
        }

        /* Configure gyroscope range */
        uint8_t gyr_range = config->gyrRange;
        ret = bmi270_write_reg(BMI270_REG_GYR_RANGE, &gyr_range, 1);
        if (ret != BMI270_OK) {
            return ret;
        }
    }

    /* Configure interrupt pins */
    /* INT1 - Active high, push-pull, output enabled */
    uint8_t int1_io_ctrl = 0x0A;  /* 0b00001010 */
    ret = bmi270_write_reg(BMI270_REG_INT1_IO_CTRL, &int1_io_ctrl, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Enable the selected sensors */
    ret = bmi270_set_sensor_enable(config->sensorEnable);

    /* Set power mode */
    ret = bmi270_set_power_mode(config->powerMode);

    sensor_initialized = true;
    return BMI270_OK;
}

/**
 * @brief Reset the BMI270 sensor
 *
 * @return int 0 on success, error code otherwise
 */
int bmi270_reset(void) {
    int ret;

    /* Send soft reset command */
    ret = bmi270_write_command(BMI270_CMD_SOFT_RESET);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Wait for reset to complete */
    delay_cycles(MS_TO_CYCLES(2));

    return BMI270_OK;
}

/**
 * @brief Enable or disable specific sensors
 *
 * @param sensor_enable Which sensors to enable
 * @return int 0 on success, error code otherwise
 */
int bmi270_set_sensor_enable(bmi270_sensor_enable_t sensor_enable) {
    int ret;
    uint8_t pwr_ctrl = 0x00;

    /* Configure sensors based on sensor_enable parameter */
    if (sensor_enable & BMI270_SENSOR_ACC) {
        pwr_ctrl |= 0x04;  /* Enable accelerometer */
    }

    if (sensor_enable & BMI270_SENSOR_GYR) {
        pwr_ctrl |= 0x02;  /* Enable gyroscope */
    }

    /* Write power control register */
    ret = bmi270_write_reg(BMI270_REG_PWR_CTRL, &pwr_ctrl, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Update current configuration */
    gCurrentConfig.sensorEnable = sensor_enable;

    return BMI270_OK;
}

/**
 * @brief Set power mode for the sensor
 *
 * @param mode Power mode to set
 * @return int 0 on success, error code otherwise
 */
int bmi270_set_power_mode(bmi270_power_mode_t mode) {
    int ret;
    uint8_t pwr_conf = 0x00;

    if (!sensor_initialized) {
        return BMI270_ERR_INVALID_PARAM;
    }

    /* Configure based on power mode */
    switch (mode) {
        case BMI270_PWR_MODE_SUSPEND:
            /* Advanced power save enabled */
            pwr_conf = 0x03;  /* Enable advanced power save */
            break;

        case BMI270_PWR_MODE_CONFIGURATION:
            /* All sensors disabled, but configuration accessible */
            pwr_conf = 0x00;  /* Disable advanced power save */
            break;

        case BMI270_PWR_MODE_LOW_POWER:
            /* Enable sensors in low power mode */
            pwr_conf = 0x03;  /* Enable advanced power save */
            break;

        case BMI270_PWR_MODE_NORMAL:
            /* Enable sensors in normal mode */
            pwr_conf = 0x02;  /* Disable advanced power save, enable FIFO self-wake */
            break;

        case BMI270_PWR_MODE_PERFORMANCE:
            /* Enable sensors in performance mode */
            pwr_conf = 0x02;  /* Disable advanced power save, enable FIFO self-wake */
            break;

        default:
            return BMI270_ERR_INVALID_PARAM;
    }

    /* Write power configuration */
    ret = bmi270_write_reg(BMI270_REG_PWR_CONF, &pwr_conf, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Wait (1ms) for settings to take effect */
    delay_cycles(MS_TO_CYCLES(1));

    /* Update current configuration */
    gCurrentConfig.powerMode = mode;

    return BMI270_OK;
}

/**
 * @brief Configure interrupt for the BMI270
 *
 * @param int_type Type of interrupt to configure
 * @param int_pin Which interrupt pin to use (INT1 or INT2)
 * @param enable true to enable, false to disable
 * @return int 0 on success, error code otherwise
 */
int bmi270_configure_interrupt(bmi270_int_type_t int_type, bmi270_int_pin_t int_pin, bool enable) {
    int ret;
    uint8_t reg_val;
    uint8_t reg_addr;
    uint8_t bit_pos;

    if (!sensor_initialized) {
        return BMI270_ERR_INVALID_PARAM;
    }

    /* Determine register address and bit position based on interrupt type */
    switch (int_type) {
        case BMI270_INT_FIFO_FULL:
        case BMI270_INT_FIFO_WM:
        case BMI270_INT_DATA_RDY:
        case BMI270_INT_ERROR:
            reg_addr = BMI270_REG_INT_MAP_DATA;
            bit_pos = int_type + (int_pin * 4);  /* 4 bits per pin in INT_MAP_DATA */
            break;

        case BMI270_INT_ANY_MOTION:
            reg_addr = (int_pin == BMI270_INT_PIN_1) ?
                      BMI270_REG_INT1_MAP_FEAT : BMI270_REG_INT2_MAP_FEAT;
            bit_pos = 6;  /* any_motion_out bit */
            break;

        case BMI270_INT_NO_MOTION:
            reg_addr = (int_pin == BMI270_INT_PIN_1) ?
                      BMI270_REG_INT1_MAP_FEAT : BMI270_REG_INT2_MAP_FEAT;
            bit_pos = 5;  /* no_motion_out bit */
            break;

        default:
            return BMI270_ERR_INVALID_PARAM;
    }

    /* Read current register value */
    ret = bmi270_read_reg(reg_addr, &reg_val, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Modify register value */
    if (enable) {
        reg_val |= (1 << bit_pos);
    } else {
        reg_val &= ~(1 << bit_pos);
    }

    /* Write updated register value */
    ret = bmi270_write_reg(reg_addr, &reg_val, 1);

    return ret;
}

/**
 * @brief Enable or disable the FIFO
 *
 * @param enable true to enable, false to disable
 * @param accel_en true to include accelerometer data in FIFO
 * @param gyro_en true to include gyroscope data in FIFO
 * @return int 0 on success, error code otherwise
 */
int bmi270_fifo_enable(bool enable, bool accel_en, bool gyro_en) {
    int ret;
    uint8_t fifo_config = 0x00;

    if (!sensor_initialized) {
        return BMI270_ERR_INVALID_PARAM;
    }

    /* Read current FIFO configuration */
    ret = bmi270_write_reg(BMI270_REG_FIFO_CONFIG_0, &fifo_config, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Read current FIFO configuration */
    ret = bmi270_read_reg(BMI270_REG_FIFO_CONFIG_1, &fifo_config, 1);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Clear bits for accelerometer and gyroscope */
    fifo_config &= ~(0xF0);  /* Clear bits 7, 6, 5 and 4 */

    /* Set bits based on parameters */
    if (enable) {
        if (accel_en) {
            fifo_config |= 0x40;  /* Set bit 6 - accelerometer */
        }
        if (gyro_en) {
            fifo_config |= 0x80;  /* Set bit 7 - gyroscope */
        }
    }

    /* Write updated FIFO configuration */
    ret = bmi270_write_reg(BMI270_REG_FIFO_CONFIG_1, &fifo_config, 1);

    return ret;
}

/**
 * @brief Read data from the FIFO
 *
 * @param buffer Buffer to store FIFO data
 * @param length Maximum length of data to read
 * @param bytes_read Actual number of bytes read
 * @return int 0 on success, error code otherwise
 */
int bmi270_fifo_read(uint8_t *buffer, uint16_t length, uint16_t *bytes_read) {
    int ret;
    uint8_t fifo_length[2];
    uint16_t fifo_bytes_available;

    if (!sensor_initialized) {
        return BMI270_ERR_INVALID_PARAM;
    }

    /* Read FIFO length */
    ret = bmi270_read_reg(BMI270_REG_FIFO_LENGTH_0, fifo_length, 2);
    if (ret != BMI270_OK) {
        return ret;
    }

    /* Calculate FIFO bytes available */
    fifo_bytes_available = ((uint16_t)fifo_length[1] << 8) | fifo_length[0];

    /* Limit read length to available bytes */
    if (length > fifo_bytes_available) {
        length = fifo_bytes_available;
    }

    /* Read FIFO data */
    if (length > 0) {
        ret = bmi270_read_reg(BMI270_REG_FIFO_DATA, buffer, length);
        if (ret != BMI270_OK) {
            return ret;
        }
    }

    *bytes_read = length;

    return BMI270_OK;
}

/**
 * @brief Set the FIFO watermark level
 *
 * @param watermark_bytes Watermark level in bytes (14-bit value, max 6144)
 * @return int 0 on success, error code otherwise
 */
int bmi270_set_fifo_watermark(uint16_t watermark_bytes)
{
    uint8_t wtm[2] = {
        (uint8_t)(watermark_bytes & 0xFF),
        (uint8_t)((watermark_bytes >> 8) & 0x3F)
    };
    return bmi270_write_reg(BMI270_REG_FIFO_WTM_0, wtm, 2);
}

/**
 * @brief Send a command to the BMI270
 *
 * @param command Command to send
 * @return int 0 on success, error code otherwise
 */
static int bmi270_write_command(uint8_t command) {
    return bmi270_write_reg(BMI270_REG_CMD, &command, 1);
}

/**
 * @brief Switch the BMI270 sensor to SPI mode
 *
 * Sends a dummy byte to enable SPI communication mode on the BMI270.
 * This must be called during initialization when the sensor is in I2C mode.
 *
 * @return void
 */
static void bmi270_switch_to_spi(void)
{
    uint8_t txBuffer[1] = {0x00};
    uint8_t rxBuffer[1];
    if (SPI_transfer(txBuffer, rxBuffer, 1,&gBMI270_SPI_CS) != 0) {
        return ;
    }

    return ;
}

/**
 * @brief Read from a BMI270 register
 *
 * @param reg_addr Register address
 * @param data Buffer to store read data
 * @param len Number of bytes to read
 * @return int 0 on success, error code otherwise
 */
static int bmi270_read_reg(uint8_t regAddr, uint8_t *data, uint16_t len) {
    /* Set read bit for SPI */
    uint8_t addr = regAddr | BMI270_SPI_READ;

    uint8_t rxBuffer[len + 2];
    uint8_t txBuffer[len + 2];

    txBuffer[0] = addr;

    if (SPI_transfer(txBuffer, rxBuffer, len + 2, &gBMI270_SPI_CS) != 0) {
        return BMI270_ERR_SPI;
    }

    /* Copy data from Rx buffer */
    memcpy(data, &rxBuffer[2], len);

    return BMI270_OK;
}

/**
 * @brief Write to a BMI270 register
 *
 * @param reg_addr Register address
 * @param data Data to write
 * @param len Number of bytes to write
 * @return int 0 on success, error code otherwise
 */
static int bmi270_write_reg(uint8_t regAddr, const uint8_t *data, uint16_t len) {
    uint8_t txBuffer[len + 1];
    /* Rx buffer to store dummy data */
    uint8_t rxBuffer[len + 1];

    /* Clear read bit for SPI */
    txBuffer[0] = regAddr & ~BMI270_SPI_READ;

    /* Copy data to buffer */
    memcpy(&txBuffer[1], data, len);

    if (SPI_transfer(txBuffer, rxBuffer, len + 1, &gBMI270_SPI_CS) != 0) {
        return BMI270_ERR_SPI;
    }

    return BMI270_OK;
}
