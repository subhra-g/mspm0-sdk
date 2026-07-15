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
#include <stdint.h>

#include "model/tvmgen_default.h"
#include "ti_msp_dl_config.h"

#include "machine_readable_code_classification_testvector.h"

#define IMAGE_MEAN      (0.5f)
#define IMAGE_SCALE     (0.5f)

#define BARCODE_INDEX   (0)
#define OTHER_INDEX     (1)
#define QR_CODE_INDEX   (2)

float if_map[1][IMG_C][IMG_H][IMG_W];
float of_map[1][NUM_CLASSES] = {{0, 0, 0}};

static void normalize_image(const float input[IMG_H][IMG_W],float output[1][IMG_C][IMG_H][IMG_W])
{
    for (uint32_t h = 0; h < IMG_H; h++)
    {
        for (uint32_t w = 0; w < IMG_W; w++)
        {
            float pixel = (float)input[h][w] / 255;
            output[0][0][h][w] = (pixel - IMAGE_MEAN) / IMAGE_SCALE;
        }
    }
}

int main(void)
{
    SYSCFG_DL_init();

    normalize_image(test_input, if_map);

    DL_NPU_reset(NPU);
    DL_NPU_enablePower(NPU);

    while (!(DL_SYSCTL_getStatus() & DL_SYSCTL_STATUS_NPU_READY));

    DL_NPU_clearInterruptStatus(NPU, DL_NPU_INTERRUPT_DONE);
    DL_NPU_enableInterrupt(NPU, DL_NPU_INTERRUPT_DONE);
    NVIC_EnableIRQ(NPU_INT_IRQn);

    struct tvmgen_default_inputs tvm_if_map = {
        (void *) &if_map[0]
    };

    struct tvmgen_default_outputs tvm_of_map = {
        (void *) &of_map[0]
    };

    tvmgen_default_run(&tvm_if_map, &tvm_of_map);

    while (!tvmgen_default_finished);

    DL_NPU_disablePower(NPU);

    uint8_t maxIndex = BARCODE_INDEX;

    if (of_map[0][OTHER_INDEX] > of_map[0][maxIndex])
    {
        maxIndex = OTHER_INDEX;
    }

    if (of_map[0][QR_CODE_INDEX] > of_map[0][maxIndex])
    {
        maxIndex = QR_CODE_INDEX;
    }

    switch (maxIndex)
    {
        case BARCODE_INDEX:
            DL_GPIO_setPins(GPIO_LED_RED_PORT, GPIO_LED_RED_PIN);
            break;

        case OTHER_INDEX:
            DL_GPIO_setPins(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN);
            break;

        case QR_CODE_INDEX:
            DL_GPIO_setPins(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN);
            break;

        default:
            break;
    }

    while (1)
    {
        __WFI();
    }
}