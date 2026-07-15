/*
 * Copyright (c) 2024, Texas Instruments Incorporated
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

#include <third_party/tinyusb/hw/bsp/board_api.h>
#include "dfu_app.h"
#include <ti/driverlib/dl_flashctl.h>

void BSL_SystemInit(void);
/*
 * @brief       Configures Flash Controller
 */
extern void BSL_Flash_Init(void);

/* Extern functions */
/*
 * @brief       The first function to get executed in the secondary BSL code
 */
extern void Reset_Handler(void);

/**
 * @brief       BSL main function.
 * Handles initialization and runs the command interpreter as super loop.
 */
int main(void)
{

    board_init();

    /* System Initialization */
    BSL_SystemInit();

    /* Initialize the Interface that will be used for BSL communication */
    dfu_app_init();

    /* Command reception phase */
    while (1) {
        tud_task();
    }
    return 0;
}

void BSL_SystemInit(void)
{
    BSL_Flash_Init();
}

void BSL_Flash_Init(void)
{
    /* Set flash wait state to 2 based on 32MHz clock*/
    DL_FlashCTL_setWaitState(FLASHCTL, (uint32_t) 2);
}

