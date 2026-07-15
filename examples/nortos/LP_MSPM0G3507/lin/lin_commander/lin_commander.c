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

static void App_processRxCallback(void);
static void App_executeCurrentState(void);

volatile bool gProcessCmd = false;
volatile OperationState_t gCurrentState = OP_STATE_PUBLISH;

/*
 * LIN Commander Message Table
 *
 * Defines the PIDs (Protected Identifiers) this commander handles.
 * Each entry contains:
 *   - msgID: 6-bit frame identifier
 *   - msgPID: Protected identifier
 *   - msgSize: Number of bytes in the message (1-8)
 *   - checksumType: CLASSIC or ENHANCED
 *   - direction: PUBLISH/SUBSCRIBE/IGNORE
 *   - callbackFunction: Optional function pointer
 */

LIN_table_record_t commanderMessageTable[LIN_COMMANDER_NUM_MSGS] = {
        /* PUBLISH frame - Commander sends data to Responder */
        {
          .msgID = 0x10,
          .msgPID = 0x00,
          .msgSize = 8,
          .checksumType = LIN_CHECKSUM_ENHANCED,
          .direction = LIN_DIRECTION_PUBLISH,
          .callbackFunction = NULL
        },
        /* SUBSCRIBE frame - Commander receives data from Responder */
        {
          .msgID = 0x20,
          .msgPID = 0x00,
          .msgSize = 8,
          .checksumType = LIN_CHECKSUM_ENHANCED,
          .direction = LIN_DIRECTION_SUBSCRIBE,
          .callbackFunction = App_processRxCallback
        },
        /* RESPONDER_TO_RESPONDER frames - Commander sends header only */
        {
          .msgID = 0x30,
          .msgPID = 0x00,
          .msgSize = 8,
          .checksumType = LIN_CHECKSUM_ENHANCED,
          .direction = LIN_DIRECTION_IGNORE,
          .callbackFunction = NULL
        },
        {
          .msgID = 0x31,
          .msgPID = 0x00,
          .msgSize = 8,
          .checksumType = LIN_CHECKSUM_ENHANCED,
          .direction = LIN_DIRECTION_IGNORE,
          .callbackFunction = NULL
        }
};

int main(void)
{
    /* Initialize all peripherals configured in SysConfig */
    SYSCFG_DL_init();

    /* Enable the LIN transceiver by setting the enable pin high */
    DL_GPIO_setPins(GPIO_LIN_ENABLE_PORT, GPIO_LIN_ENABLE_USER_LIN_ENABLE_PIN);

    /* Calculate PIDs for all messages */
    for (uint8_t msg = 0; msg < LIN_COMMANDER_NUM_MSGS; msg++)
    {
        commanderMessageTable[msg].msgPID = LIN_calculatePID(commanderMessageTable[msg].msgID);
    }

    /* Link message table to global LIN context */
    gLIN.cmdMsgTbl = commanderMessageTable;

    /* Initialize TX buffer with test data */
    for (uint8_t testDataIndex = 0; testDataIndex < LIN_DATA_MAX_BUFFER_SIZE; testDataIndex++)
    {
        gLIN.txBuf[testDataIndex] = testDataIndex;
    }

    /* Clear any pending interrupts and enable LIN UART interrupt */
    NVIC_ClearPendingIRQ(LIN_0_INST_INT_IRQN);
    NVIC_EnableIRQ(LIN_0_INST_INT_IRQN);

    /* Clear any pending interrupts and enable timer interrupt */
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);

    /* Enable switch interrupts */
    NVIC_EnableIRQ(GPIO_SWITCHES1_INT_IRQN);

    /* Initial LED state - both off */
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);

    /* Main loop - Sequential operation demo */
    while (1)
    {
        /* Button press triggers next operation */
        if (gProcessCmd)
        {
            gProcessCmd = false;
            App_executeCurrentState();

            /* Advance to next state */
            gCurrentState++;
            if (gCurrentState >= OP_STATE_MAX)
            {
                gCurrentState = OP_STATE_PUBLISH;
            }
        }
        else
        {
            /* Low power wait */
            __WFI();
        }
    }
}

static void App_executeCurrentState(void)
{
    switch (gCurrentState)
    {
    case OP_STATE_PUBLISH:
        LIN_Commander_sendFrame(LIN_0_INST, TIMER_0_INST, MSG_IDX_PUBLISH, gLIN.txBuf, commanderMessageTable);
        while (gLIN.transactionCompleted != true);
        /* LED1 pulse for TX */
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        delay_cycles(DELAY_1MS * 50);
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        break;

    case OP_STATE_SUBSCRIBE:
        LIN_Commander_sendFrame(LIN_0_INST, TIMER_0_INST, MSG_IDX_SUBSCRIBE, gLIN.txBuf, commanderMessageTable);
        while (gLIN.transactionCompleted != true);
        /*
         * There is no LED indication here because a callback function will be triggered on successful RX of data
         * which will blink LED2.
         */
        break;

    case OP_STATE_RESP_TO_RESP_1:
        LIN_Commander_sendFrame(LIN_0_INST, TIMER_0_INST, MSG_IDX_R2R_1, gLIN.txBuf, commanderMessageTable);
        while (gLIN.transactionCompleted != true);
        /* LED1 pulse for header TX */
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        delay_cycles(DELAY_1MS * 50);
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        break;

    case OP_STATE_RESP_TO_RESP_2:
        LIN_Commander_sendFrame(LIN_0_INST, TIMER_0_INST, MSG_IDX_R2R_2, gLIN.txBuf, commanderMessageTable);
        while (gLIN.transactionCompleted != true);
        /* LED1 pulse for header TX */
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        delay_cycles(DELAY_1MS * 50);
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        break;

    case OP_STATE_SLEEP:
        LIN_Commander_sendGoToSleep(LIN_0_INST);
        while (gLIN.transactionCompleted != true);
        /* Turn both LEDs on briefly to indicate sleep */
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);
        delay_cycles(DELAY_1MS * 500);
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);
        delay_cycles(OPERATION_DELAY_CYCLES);
        break;

    case OP_STATE_WAKEUP:
        LIN_Commander_sendWakeup(LIN_0_INST);
        while (gLIN.transactionCompleted != true);
        /* Turn both LEDs on briefly to indicate wake-up */
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);
        delay_cycles(DELAY_1MS * 100);
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);
        break;

    case OP_STATE_SYNC_ERROR:
        /* Wrong sync byte - 0xAA instead of 0x55 */
        DL_UART_enableLINSendBreak(LIN_0_INST);
        delay_cycles(LIN_BREAK_LENGTH);
        DL_UART_disableLINSendBreak(LIN_0_INST);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, 0xAA);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, commanderMessageTable[MSG_IDX_PUBLISH].msgPID);
        while (DL_UART_isBusy(LIN_0_INST));
        while (!DL_UART_isRXFIFOEmpty(LIN_0_INST))
        {
            DL_UART_Extend_receiveData(LIN_0_INST);
        }
        break;

    case OP_STATE_CHKSUM_ERROR:
        /* Wrong checksum */
        DL_UART_enableLINSendBreak(LIN_0_INST);
        delay_cycles(LIN_BREAK_LENGTH);
        DL_UART_disableLINSendBreak(LIN_0_INST);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, LIN_SYNC_BYTE);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, commanderMessageTable[MSG_IDX_PUBLISH].msgPID);
        while (DL_UART_isBusy(LIN_0_INST));
        while (!DL_UART_isRXFIFOEmpty(LIN_0_INST))
        {
            DL_UART_Extend_receiveData(LIN_0_INST);
        }
        for (uint8_t i = 0; i < 8; i++)
        {
            DL_UART_Extend_transmitDataBlocking(LIN_0_INST, gLIN.txBuf[i]);
        }
        /* Wrong checksum */
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, 0x00);
        while (DL_UART_isBusy(LIN_0_INST));
        while (!DL_UART_isRXFIFOEmpty(LIN_0_INST))
        {
            DL_UART_Extend_receiveData(LIN_0_INST);
        }
        break;

    case OP_STATE_PID_PARITY_ERROR:
        /* Bad PID parity - flip parity bits */
        DL_UART_enableLINSendBreak(LIN_0_INST);
        delay_cycles(LIN_BREAK_LENGTH);
        DL_UART_disableLINSendBreak(LIN_0_INST);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, LIN_SYNC_BYTE);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, commanderMessageTable[MSG_IDX_PUBLISH].msgPID ^ 0xC0);
        while (DL_UART_isBusy(LIN_0_INST));
        while (!DL_UART_isRXFIFOEmpty(LIN_0_INST))
        {
            DL_UART_Extend_receiveData(LIN_0_INST);
        }
        break;

    case OP_STATE_COM_NO_RES_ERROR:
        /* Unknown PID - no responder will reply */
        DL_UART_enableLINSendBreak(LIN_0_INST);
        delay_cycles(LIN_BREAK_LENGTH);
        DL_UART_disableLINSendBreak(LIN_0_INST);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, LIN_SYNC_BYTE);
        DL_UART_Extend_transmitDataBlocking(LIN_0_INST, LIN_calculatePID(0x15));
        while (DL_UART_isBusy(LIN_0_INST));
        while (!DL_UART_isRXFIFOEmpty(LIN_0_INST))
        {
            DL_UART_Extend_receiveData(LIN_0_INST);
        }
        /*
         * Enable RX interrupt and TIMER to receive RX data and generate TIMEOUT error
         * if not byte is received.
         */
        gLIN.transactionCompleted = false;
        gLIN.rxIdx = 0;
        gLIN.chkSum.word = 0;
        gLIN.rxState = LIN_RX_STATE_DATA;
        gLIN.msgTblIdx = MSG_IDX_SUBSCRIBE;
        DL_UART_Extend_clearInterruptStatus(LIN_0_INST,DL_UART_EXTEND_INTERRUPT_RX);
        DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);
        DL_Timer_setLoadValue(TIMER_0_INST, TIMEOUT);
        DL_Timer_startCounter(TIMER_0_INST);
        while (gLIN.transactionCompleted != true);
        break;

    case OP_STATE_COM_INCMPLT_RES_ERROR:
        commanderMessageTable[MSG_IDX_SUBSCRIBE].msgSize = 9;
        LIN_Commander_sendFrame(LIN_0_INST, TIMER_0_INST, MSG_IDX_SUBSCRIBE, gLIN.txBuf, commanderMessageTable);
        while (gLIN.transactionCompleted != true);
        commanderMessageTable[MSG_IDX_SUBSCRIBE].msgSize = 8;
        break;

    case OP_STATE_RES_NO_RES_ERROR:
        commanderMessageTable[MSG_IDX_PUBLISH].msgSize = 0;
        LIN_Commander_sendFrame(LIN_0_INST, TIMER_0_INST, MSG_IDX_PUBLISH, gLIN.txBuf, commanderMessageTable);
        while (gLIN.transactionCompleted != true);
        commanderMessageTable[MSG_IDX_PUBLISH].msgSize = 8;
        break;

    case OP_STATE_RES_INCMPLT_RES_ERROR:
        commanderMessageTable[MSG_IDX_PUBLISH].msgSize = 5;
        LIN_Commander_sendFrame(LIN_0_INST, TIMER_0_INST, MSG_IDX_PUBLISH, gLIN.txBuf, commanderMessageTable);
        while (gLIN.transactionCompleted != true);
        commanderMessageTable[MSG_IDX_PUBLISH].msgSize = 8;
        break;

    default:
        gCurrentState = OP_STATE_PUBLISH;
        break;
    }
}

void GROUP1_IRQHandler(void)
{
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1))
    {
    case GPIO_SWITCHES1_INT_IIDX:
        switch (DL_GPIO_getPendingInterrupt(GPIO_SWITCHES1_PORT))
        {
        case GPIO_SWITCHES1_USER_SWITCH_1_IIDX:
            gProcessCmd = true;
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Callback function for successful RX (SUBSCRIBE frames)
 */
static void App_processRxCallback(void)
{
    /* LED2 pulse for RX */
    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_2_PIN);
    delay_cycles(DELAY_1MS * 50);
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_2_PIN);
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
        delay_cycles(CPUCLK_FREQ / 20);
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_LED_2_PIN);
        delay_cycles(CPUCLK_FREQ / 20);
    }
}
