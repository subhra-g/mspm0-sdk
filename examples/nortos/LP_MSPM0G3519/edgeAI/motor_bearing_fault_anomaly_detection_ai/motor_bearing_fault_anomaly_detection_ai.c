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

#include "ti_msp_dl_config.h"

#include "model/tvmgen_default.h"
#include "feature_extract.h"

/* ARM CMSIS-DSP Header files */
#include "arm_const_structs.h"
#include "arm_math.h"
#include "stdlib.h"

#include "bearing_fault_class_data.h"

/** \brief Model input buffer */
int8_t if_map[1][3][128*2][1];

/** \brief Output vector */
float output_vector[1][3][128][1];

/** \brief Extracted features of a single frame */
int8_t totalFeatures[FE_VARIABLES][FE_NUM_FRAME_CONCAT*FE_FEATURE_SIZE_PER_FRAME*2];

/** \brief TVM output structure for model results */
struct tvmgen_default_outputs outputs = {(void*)&output_vector[0]};


/** \brief Glow LED based on which class has the maximum frequency */
void glow_led()
{
    /* Get the current class data pointer */
    const int32_t *current_data = get_current_bearing_fault_class_data();

    /* Find reconstruction error */
    float error = 0;
    int32_t index = 0;
    for(int i=0;i<FE_VARIABLES;i++)
    {
        for(int j=0;j<FE_FEATURE_SIZE_PER_FRAME;j++)
        {
            error += (float)((float)(((float)current_data[index] - output_vector[0][i][j][0]))*((float)((float)current_data[index] - output_vector[0][i][j][0])));
            index++;
        }
    }
    error = (float)(error / (FE_NN_OUT_SIZE));

    uint8_t output_class = BEARING_FAULT_CLASS_1_NORMAL;
    /* Set as anomaly if error is greater than or equal to threshold */
    if (error >  RECONSTRUCTION_ERROR_THRESHOLD)
    {
        output_class = BEARING_FAULT_CLASS_2_ANOMALY;
    }

    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_LED1_PIN);
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_LED2_PIN);
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_LED3_PIN);
    /* Glow respective class */
    switch(output_class) {
        case BEARING_FAULT_CLASS_1_NORMAL:
            DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_LED3_PIN);
            break;
        case BEARING_FAULT_CLASS_2_ANOMALY:
            DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_LED2_PIN);
            break;
        default:
            break;
    }
}

/**
 * \brief Main application function
 *
 * Initializes hardware, processes Bearing Fault data directly from class data arrays
 * through the AI model for classification.
 */
int main(void)
{
    /* Input to model */
    struct tvmgen_default_inputs inputs;

    SYSCFG_DL_init();

    /* Initialize ARM library for real FFT */
    FE_init();

    /* Main processing loop */
    while (1) {
        /* Get the current class data pointer */
        const int32_t *current_data = get_current_bearing_fault_class_data();

        /* Feature extraction */
        int16_t data[FE_FRAME_SIZE];
        int32_t index = 0;
        for(int i=0;i<FE_VARIABLES;i++)
        {
            for(int j=0;j<FE_FRAME_SIZE;j++)
            {
                data[j] = (int16_t) (current_data[index]);
                index++;
            }
            FE_process((q15_t *)data, totalFeatures[i], 0);
        }
        for(int i=0;i<FE_VARIABLES;i++)
        {
            for(int j=0;j<FE_FEATURE_SIZE_PER_FRAME;j++)
            {
                if_map[0][i][j][0] = totalFeatures[i][j];
            }
        }

        /* Set up input to point directly to the class data */
        inputs.x = (void *)&if_map;

        /* Run inference directly on the class data */
        (void)tvmgen_default_run(&inputs, &outputs);

        /* Glow the respective led for the class with maximum output */
        glow_led();

        /* For continuous operation, delay then update to next class */
        delay_cycles(10000000);
    }
}
