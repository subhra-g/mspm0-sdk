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

#include "lin_config.h"

static void LIN_processMessage_Rx(void);

/*
 * LIN Responder Message Table
 *
 * Defines the PIDs (Protected Identifiers) this responder handles.
 * Each entry contains:
 *   - msgID: 6-bit frame identifier
 *   - msgPID: Protected identifier (calculated at init)
 *   - msgSize: Number of bytes in the message (1-8)
 *   - checksumType: CLASSIC or ENHANCED
 *   - direction: PUBLISH/SUBSCRIBE
 *   - callbackFunction: Optional function pointer
 */

LIN_table_record_t responderMessageTable[LIN_RESPONDER_NUM_MSGS] = {
    /* SUBSCRIBE frame - Responder receives data from Commander */
    {
        .msgID = 0x10,
        .msgPID = 0x00,
        .msgSize = 8,
        .checksumType = LIN_CHECKSUM_ENHANCED,
        .direction = LIN_DIRECTION_SUBSCRIBE,
        .callbackFunction = LIN_processMessage_Rx
    },
    /* PUBLISH frame - Responder sends data to Commander */
    {
        .msgID = 0x20,
        .msgPID = 0x00,
        .msgSize = 8,
        .checksumType = LIN_CHECKSUM_ENHANCED,
        .direction = LIN_DIRECTION_PUBLISH,
        .callbackFunction = NULL
    },
    /* Responder_TO_Responder frame - Responder sends data to another responder */
    {
        .msgID = 0x30,
        .msgPID = 0x00,
        .msgSize = 8,
        .checksumType = LIN_CHECKSUM_ENHANCED,
        .direction = LIN_DIRECTION_PUBLISH,
        .callbackFunction = NULL
    },
};

int main(void)
{
    uint8_t idx;

    /* Initialize all peripherals configured in SysConfig */
    SYSCFG_DL_init();

    /* Enable the LIN transceiver by setting the enable pin high */
    DL_GPIO_setPins(GPIO_LIN_ENABLE_PORT, GPIO_LIN_ENABLE_USER_LIN_ENABLE_PIN);

    /* Calculate PIDs for all messages */
    for (idx = 0; idx < LIN_RESPONDER_NUM_MSGS; idx++)
    {
        responderMessageTable[idx].msgPID = LIN_calculatePID(responderMessageTable[idx].msgID);
    }

    /* Link message table to global LIN context */
    gLIN.respMsgTbl = responderMessageTable;

    /* Initialize TX buffer with test data */
    for (idx = 0; idx < LIN_DATA_MAX_BUFFER_SIZE; idx++)
    {
        gLIN.txBuf[idx] = idx;
    }

    /* Initialize LINC0 counter compare value with minimum possible break field duration */
    DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_BREAK_FIELD_MIN_CYCLES);

    /* Clear any pending interrupts and enable LIN UART interrupt */
    NVIC_ClearPendingIRQ(LIN_0_INST_INT_IRQN);
    NVIC_EnableIRQ(LIN_0_INST_INT_IRQN);

    /* Clear any pending interrupts and enable timer interrupt */
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);

    /* Enable automatic sleep when exiting ISR for low power operation */
    DL_SYSCTL_enableSleepOnExit();

    /* Initial LED state - both off (waiting for bus activity) */
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);

    /* Main loop: sleep until interrupt occurs */
    while (1)
    {
        /* Wait For Interrupt - enters low power mode */
        __WFI();
    }
}

/**
 * @brief LIN Receive Message Handler
 *
 * Called as a callback func when a LIN message is received (SUBSCRIBE frame).
 * Copies received data to the transmit buffer (with modification) and
 * toggles LED1 to provide visual indication of reception.
 */
static void LIN_processMessage_Rx(void)
{
    uint8_t i;

    /* Copy received data to transmit buffer with modification (echo + 1) */
    for (i = 0; i < LIN_DATA_MAX_BUFFER_SIZE; i++)
    {
        gLIN.txBuf[i] = gLIN.rxBuf[i] + 1;
    }

    /* Toggle LED1 to indicate a packet has been received */
    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    delay_cycles(DELAY_1MS * 50);
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
}

/**
 * @brief LIN Transmit Message Handler
 *
 * Called after a LIN message has been transmitted (PUBLISH frame).
 * Toggles LED2 to provide visual indication of transmission.
 * Increments TX data for next transmission.
 */
void LIN_processMessage_Tx(void)
{
    /* Increment TX data for next transmission */
    gLIN.txBuf[0]++;

    /* Toggle LED2 to indicate a packet has been transmitted */
    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_2_PIN);
    delay_cycles(DELAY_1MS * 50);
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_2_PIN);
}

void LIN_processWakeUp(void)
{
    /*
     * User can enter whatever he wants to do while bus is coming back to active mode.
     */
}

void LIN_processSleep(void)
{
    /*
     * User can enter whatever he wants to do while bus is going to sleep.
     */
}

/**
 * @brief LIN Error Handler
 *
 * Called when a LIN error is detected.
 *
 * @param error The error type that occurred
 */
void LIN_processError(LIN_ERROR error)
{
    /* Visual indication: Both LEDs blink 3 times to indicate error*/
    for (uint8_t i = 0; i < 3; i++)
    {
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);
        delay_cycles(CPUCLK_FREQ / 20);  /* 50ms on */
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);
        delay_cycles(CPUCLK_FREQ / 20);  /* 50ms off */
    }
}
