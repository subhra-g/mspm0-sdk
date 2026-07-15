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

#include <stdio.h>
#include <string.h>
#include "FatFS/diskio.h"
#include "FatFS/ff.h"
#include "ti_msp_dl_config.h"

FATFS fs;
FIL file;
FRESULT fr;
UINT bw;

uint8_t test;

int main(void)
{
    SYSCFG_DL_init();
    delay_cycles(32000000);

    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);

    test = disk_initialize(0);
    if ((test & 0x01) != 0) {
        while (1)
            ;
    }

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        while (1)
            ;
    }

    fr = f_open(&file, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        while (1)
            ;
    }

    const char *text = "Hello World!!!\r\n";
    fr               = f_write(&file, text, strlen(text), &bw);
    if (fr != FR_OK) {
        while (1)
            ;
    }

    f_close(&file);

    f_mount(NULL, "", 1);

    while (1) {
    }
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerA_getPendingInterrupt(TIMER_0_INST)) {
        case DL_TIMERA_IIDX_LOAD:
            disk_timerproc();
            break;
        default:
            break;
    }
}
