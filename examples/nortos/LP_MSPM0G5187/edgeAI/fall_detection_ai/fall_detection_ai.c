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

#include "model/tvmgen_default.h"
#include "model/user_input_config.h"
#include "ti_msp_dl_config.h"
#include "imu/sensor_BMI270.h"
#include "feature_extract.h"
#include <string.h>

#define COLOR_RED   ((uint8_t)0x1)
#define COLOR_BLUE  ((uint8_t)0x2)
#define COLOR_GREEN ((uint8_t)0x4)
#define COLOR_WHITE (COLOR_BLUE | COLOR_GREEN | COLOR_RED)

#define IDX_ADL  0
#define IDX_FALL 1

/* BMI270 FIFO accelerometer frame: 6 bytes (x,y,z each 2 bytes) */
#define BMI270_FIFO_ACC_FRAME_LEN 6

/* Fall is confirmed only when at least 5 out of 8 inferences detect a fall */
#define INFERENCE_FRAME_THRESHOLD_FOR_FALL 5

/* Watermark in bytes: FE_FRAME_SIZE samples × 6 bytes/sample */
#define FIFO_WATERMARK_BYTES (FE_FRAME_SIZE * BMI270_FIFO_ACC_FRAME_LEN)

/* Raw FIFO read buffer */
static uint8_t gFIFORaw[FIFO_WATERMARK_BYTES];

/* Single parsed sample buffer */
static int16_t gSamples[FE_VARIABLES][FE_FRAME_SIZE];

/* Model I/O */
static int8_t if_map[1][FE_VARIABLES][FE_STACKING_FRAME_WIDTH][1];
/* Currently the NPU outputs 32-bit word-aligned data */
static int8_t of_map[1][4] = {{0, 0}};
static int8_t featuresPerFrame[FE_FEATURE_SIZE_PER_FRAME];

/* Set by FIFO watermark interrupt; cleared by main after burst-reading FIFO */
static volatile bool gCheckSPI __attribute__((aligned(4))) = false;

/* Circular buffer to store the last 8 predictions from the neural network.
   Each prediction is 0 (normal) or 1 (fall).*/
static volatile uint8_t gPredVector[8] = {0, 0, 0, 0, 0, 0, 0, 0};

/* Write pointer for the circular prediction buffer.*/
static volatile uint8_t gPredPtr = 0;

/* Running sum of the values in the 'gPredVector' buffer, used 
   for a voting-based fall detection algorithm.*/
static volatile uint16_t gPredSum = 0;

static void BMI270_sensor_init(void);
static void parse_fifo_accel(const uint8_t *rawBuf, uint16_t rawLen, int16_t samples[FE_VARIABLES][FE_FRAME_SIZE]);
static void glow_led(uint8_t color);
static void clear_led(uint8_t color);
static int post_processing(int newInference);

/* Main application entry point */
int main(void)
{
    SYSCFG_DL_init();
    DL_SPI_enable(SPI_INST);

    FE_init();

    struct tvmgen_default_inputs tvm_if_map = {(void *) &if_map[0]};
    struct tvmgen_default_outputs tvm_of_map = {(void *) &of_map[0]};

    /* Configure BMI270: FIFO mode, acc-only, watermark = FE_FRAME_SIZE samples */
    BMI270_sensor_init();
    NVIC_EnableIRQ(SENSOR_INTR_INT_IRQN);

    while (1)
    {
        /* Block until the FIFO has accumulated the next FE_FRAME_SIZE samples */
        while (false == gCheckSPI)
        {
            __WFE();
        }
        gCheckSPI = false;

        /* Burst-read FE_FRAME_SIZE acc samples from FIFO via SPI */
        uint16_t bytesRead = 0;
        bmi270_fifo_read(gFIFORaw, FIFO_WATERMARK_BYTES, &bytesRead);

        /* Parse raw FIFO byte stream into structured int16 x/y/z arrays */
        parse_fifo_accel(gFIFORaw, bytesRead, gSamples);

        /* Feature extraction and rolling window update */
        for (int var = 0; var < FE_VARIABLES; var++)
        {
            FE_process(gSamples[var], featuresPerFrame, var);

            /* Slide the feature window left by one frame's worth of features */
            memmove(&if_map[0][var][0][0],
                    &if_map[0][var][FE_FEATURE_SIZE_PER_FRAME][0],
                    (FE_STACKING_FRAME_WIDTH - FE_FEATURE_SIZE_PER_FRAME) * sizeof(int8_t));

            /* Append the new features at the end of the window */
            memcpy(&if_map[0][var][FE_STACKING_FRAME_WIDTH - FE_FEATURE_SIZE_PER_FRAME][0],
                   featuresPerFrame,
                   FE_FEATURE_SIZE_PER_FRAME * sizeof(int8_t));
        }

        /* Run inference every frame */
        tvmgen_default_run(&tvm_if_map, &tvm_of_map);

        if (post_processing(of_map[0][IDX_FALL] - of_map[0][IDX_ADL]))
        {
            glow_led(COLOR_RED);
        }
        else
        {
            glow_led(COLOR_GREEN);
        }
    }
    return 0;
}

/* Handles sensor interrupt indicating FIFO data is ready */
void GROUP1_IRQHandler(void)
{
    switch (DL_GPIO_getPendingInterrupt(GPIOA))
    {
        case SENSOR_INTR_ACC_GYRO_IIDX:
            DL_GPIO_clearInterruptStatus(SENSOR_INTR_PORT, SENSOR_INTR_ACC_GYRO_PIN);
            gCheckSPI = true;
            break;
        default:
            break;
    }
}

/**
 * @brief Configure BMI270 in FIFO mode with watermark interrupt.
 *
 * The sensor writes FE_FRAME_SIZE accelerometer samples to its FIFO at the
 * configured ODR. When the byte count reaches FIFO_WATERMARK_BYTES the INT1
 * pin fires, waking the main loop to do a burst SPI read.
 */
static void BMI270_sensor_init(void)
{
    int ret;
    bmi270_config_t config = {
        .powerMode     = BMI270_PWR_MODE_NORMAL,
        .sensorEnable  = BMI270_SENSOR_ACC,
        .accRange      = BMI270_ACC_RANGE_16G,
        .accOdr        = BMI270_ODR_200HZ,
        .accBwp        = BMI270_BWP_NORMAL,
        .accFilterPerf = true,
    };

    ret = bmi270_init(&config);
    if (ret != BMI270_OK) { __BKPT(); }

    ret = bmi270_fifo_enable(true, true, false);
    if (ret != BMI270_OK) { __BKPT(); }

    ret = bmi270_set_fifo_watermark(FIFO_WATERMARK_BYTES);
    if (ret != BMI270_OK) { __BKPT(); }

    ret = bmi270_configure_interrupt(BMI270_INT_FIFO_WM, BMI270_INT_PIN_1, true);
    if (ret != BMI270_OK) { __BKPT(); }
}

/**
 * @brief Parse a raw BMI270 FIFO byte stream into structured x/y/z sample arrays.
 *
 * Stops when FE_FRAME_SIZE samples have been extracted or the buffer is exhausted.
 * Since the sensor outputs the latest sample first, the order of the frames are reversed.
 */
static void parse_fifo_accel(const uint8_t *rawBuf, uint16_t rawLen, int16_t samples[FE_VARIABLES][FE_FRAME_SIZE])
{
    uint16_t idx         = 0;
    uint16_t sampleCount = 0;

    while (idx < rawLen && sampleCount < FE_FRAME_SIZE)
    {
        if ((idx + 6u) > rawLen) { break; }

        samples[0][FE_FRAME_SIZE - 1 - sampleCount] = (int16_t)((uint16_t)rawBuf[idx]     | ((uint16_t)rawBuf[idx + 1] << 8));
        samples[1][FE_FRAME_SIZE - 1 - sampleCount] = (int16_t)((uint16_t)rawBuf[idx + 2] | ((uint16_t)rawBuf[idx + 3] << 8));
        samples[2][FE_FRAME_SIZE - 1 - sampleCount] = (int16_t)((uint16_t)rawBuf[idx + 4] | ((uint16_t)rawBuf[idx + 5] << 8));
        idx += 6;
        sampleCount++;
    }
}

/* Post-process inference results using temporal voting.
 * Maintains a buffer of last 8 inferences and returns 1 (fall)
 * when at least 5 of the last 8 inferences indicate a fall. */
static int post_processing(int newInference)
{
    /* Threshold for considering an inference as a fall (0 = any non-negative value) */
    int inference_threshold = 0;
    /* Fall is confirmed when at least 5 out of 8 inferences detect a fall */
    int frame_threshold = INFERENCE_FRAME_THRESHOLD_FOR_FALL;

    /* Remove oldest prediction from sum */
    gPredSum -= gPredVector[gPredPtr];
    /* Store new inference in circular buffer (1 = fall, 0 = no fall) */
    if (newInference >= inference_threshold)
    {
        gPredVector[gPredPtr] = 1;
        gPredSum += 1;
    }
    else
    {
        gPredVector[gPredPtr] = 0;
    }
    /* Advance circular buffer pointer */
    gPredPtr = (gPredPtr + 1) % 8;
    /* Return 1 if enough recent inferences indicate a fall */
    if (gPredSum >= frame_threshold)
    {
        return 1;
    }
    return 0;
}

/* Turn on LED(s) specified by color bitmask */
static void glow_led(uint8_t color)
{
    clear_led(COLOR_WHITE);
    if (color & COLOR_RED)   { DL_GPIO_setPins(GPIO_LED_RED_PORT,   GPIO_LED_RED_PIN);   }
    if (color & COLOR_BLUE)  { DL_GPIO_setPins(GPIO_LED_BLUE_PORT,  GPIO_LED_BLUE_PIN);  }
    if (color & COLOR_GREEN) { DL_GPIO_setPins(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN); }
}

/* Turn off LED(s) specified by color bitmask */
static void clear_led(uint8_t color)
{
    if (color & COLOR_RED)   { DL_GPIO_clearPins(GPIO_LED_RED_PORT,   GPIO_LED_RED_PIN);   }
    if (color & COLOR_BLUE)  { DL_GPIO_clearPins(GPIO_LED_BLUE_PORT,  GPIO_LED_BLUE_PIN);  }
    if (color & COLOR_GREEN) { DL_GPIO_clearPins(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN); }
}
