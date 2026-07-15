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

#include "model/tvmgen_default.h"
#include "model/user_input_config.h"
#include "ti_msp_dl_config.h"
#include "sensor.h"


#define NUM_OUTPUT_CLASS 4

/**
 * \brief Model output buffer.
 * Stores the 4 classification scores (int8_t) from the NPU inference.
 */
volatile int8_t of_map[1][4] __attribute__((aligned(4))) = {{0,0,0,0}};

/**
 * \brief Input data configuration.
 * FE_FRAME_SIZE defines the frame size (256 time steps).
 * input_arr stores X, Y, and Z accelerometer axes.
 */
volatile float input_arr[FE_VARIABLES][FE_FRAME_SIZE];

/**
 * \brief Normalized input data configuration.
 * num_samples defines the window size (256 time steps).
 * input_arr stores X, Y, and Z accelerometer axes.
 */
volatile int8_t normalized_input_arr[FE_VARIABLES][FE_FRAME_SIZE];

/* TVM interface structures linking the buffers to the generated model code */
struct tvmgen_default_inputs tvm_if_map = {(void*) &normalized_input_arr[0]};
struct tvmgen_default_outputs tvm_of_map = {(void*) &of_map[0]};

/* Global variable to store the result of the classification (0 to 3) */
volatile int8_t gOutputClass = 0;


/**
 * \brief Identifies the index of the highest value in the output map.
 */
void find_output_class() 
{

    int8_t output_class = 0;

    for(int i = 1; i < NUM_OUTPUT_CLASS; i++) 
    {
        if(of_map[0][i] > of_map[0][output_class]) 
        {
            output_class = i;
        }
    }

    gOutputClass = output_class;
}


/**
 * \brief Updates hardware LEDs and modifies data sliding window based on result.
 */
void glow_led()
{

    /* Turn off all LEDs before setting the new state */
    DL_GPIO_clearPins(LED_GREEN_PORT, LED_GREEN_PIN);
    DL_GPIO_clearPins(LED_BLUE_PORT, LED_BLUE_PIN);
    DL_GPIO_clearPins(LED_RED_PORT, LED_RED_PIN);

    switch(gOutputClass)
    {
        case 0: /* Circle gesture detected */
            DL_GPIO_setPins(LED_GREEN_PORT, LED_GREEN_PIN);
            gInputIndex = 0;
            break;
        case 1: /* Wave gesture detected  */
            DL_GPIO_setPins(LED_BLUE_PORT, LED_BLUE_PIN);
            gInputIndex = 0;
            break;
        case 2: /* Tap gesture detected  */
            DL_GPIO_setPins(LED_RED_PORT, LED_RED_PIN);
            gInputIndex = 0;
            break;
        case 3: /* Other class detected  */
            gInputIndex = gInputIndex - 64 ;
            break;
        default:
            break;
    }

}

void FE_Normalize()
{
    float inputVal = 0;
    int32_t inputVal_q31 = 0;
    for(int input_feature = 0; input_feature < FE_FEATURE_SIZE_PER_FRAME; input_feature++)
    {
        for(uint8_t axis = 0; axis < FE_VARIABLES; axis++)
        {
            inputVal = input_arr[axis][input_feature];

            inputVal_q31 = ((int32_t)((inputVal + tvmgen_default_bias_data[axis]) * tvmgen_default_scale_data[axis])) \
                    >> tvmgen_default_shift_data[axis];

            if(inputVal_q31 > 127)
            {
                inputVal_q31 = 127;
            }
            else if(inputVal_q31 < -128)
            {
                inputVal_q31 = -128;
            }

            normalized_input_arr[axis][input_feature] = (int8_t) inputVal_q31;
        }
    }
}

int main(void)
{
    SYSCFG_DL_init();
    NVIC_EnableIRQ(GPIOA_INT_IRQn);

    /* Initialize SPI communication for the sensor */
    DL_SPI_enable(SPI_INST);

    /* Power up the neural processing unit (NPU) module.
     * Clear and enable the NPU interrupt.
     */
    DL_NPU_reset(NPU);
    DL_NPU_enablePower(NPU);
    while (!(DL_SYSCTL_getStatus() & DL_SYSCTL_STATUS_NPU_READY));
    DL_NPU_clearInterruptStatus(NPU, DL_NPU_INTERRUPT_DONE);
    DL_NPU_enableInterrupt(NPU, DL_NPU_INTERRUPT_DONE);
    NVIC_EnableIRQ(NPU_INT_IRQn);

    /* --- BMI270 Sensor Configuration --- */
    int ret;
    bmi270_config_t config;
    config.powerMode = BMI270_PWR_MODE_NORMAL;
    config.sensorEnable = BMI270_SENSOR_ACC;  //Enable only accelerometer
    config.accRange = BMI270_ACC_RANGE_8G;
    config.accOdr = BMI270_ODR_100HZ;
    config.accBwp = BMI270_BWP_NORMAL;
    config.accFilterPerf = true;

    /* Initialize the sensor */
    ret = bmi270_init(&config);
    if(ret != BMI270_OK) 
    {
        __BKPT();
    }

    /* Configure data ready interrupt on INT1 pin */
    ret = bmi270_configure_interrupt(BMI270_INT_DATA_RDY, BMI270_INT_PIN_1, true);
    if(ret != BMI270_OK) 
    {
        __BKPT();
    }

    gInputIndex = 0;

    while(1){

        /* Enable GPIO interrupt to trigger on sensor data ready */
        DL_GPIO_clearInterruptStatus(SENSOR_INTR_PORT, SENSOR_INTR_ACC_GYRO_PIN);
        DL_GPIO_enableInterrupt(SENSOR_INTR_PORT, SENSOR_INTR_ACC_GYRO_PIN);

        /* Blocking call: Fills input_arr with 256 samples from the sensor */
        Sensor_Accelerometer_captureData(input_arr);
        FE_Normalize();

        /* Disable interrupt to prevent triggers during inference processing */
        DL_GPIO_disableInterrupt(SENSOR_INTR_PORT, SENSOR_INTR_ACC_GYRO_PIN);
        DL_GPIO_clearInterruptStatus(SENSOR_INTR_PORT, SENSOR_INTR_ACC_GYRO_PIN);

        /* Run Inference using NPU */
        tvmgen_default_run(&tvm_if_map, &tvm_of_map);

        /* Wait for NPU inference to complete */
        while(!tvmgen_default_finished);

        /* determine result */
        find_output_class();
        glow_led();

        /* --- Sliding Window Management --- */
        /* Move the last 192 samples to the front of the array. */
        /* This allows the next capture to only need 64 new samples (75% overlap). */
        for(uint8_t axis = 0; axis < FE_VARIABLES; axis++)
        {
            memmove(&input_arr[axis][0], &input_arr[axis][64], 192 * sizeof(float));
        }

    }

    /* Power down the NPU after inference */
    DL_NPU_disablePower(NPU);
    return(0);
}


/* GPIO interrupt handler for sensor ready events */
void GROUP1_IRQHandler(void)
{
    switch(DL_GPIO_getPendingInterrupt(GPIOA))
    {
        case SENSOR_INTR_ACC_GYRO_IIDX:
                DL_GPIO_clearInterruptStatus(SENSOR_INTR_PORT, SENSOR_INTR_ACC_GYRO_PIN);
                gDataReady_Acc_Gyro = true;
                break;

        default:
                break;
    }
}
