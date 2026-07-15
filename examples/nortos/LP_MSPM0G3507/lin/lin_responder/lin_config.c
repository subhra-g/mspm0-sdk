/*
 * Copyright (c) 2026, Texas Instruments Incorporated - http://www.ti.com
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

volatile uint8_t rxByte = 0, syncStartBit = 0;

/* Timing and auto-baud rate context with default values */
volatile Lin_timing_t gLINT = {
    /* Break field timing */
    .brkW = 0,
    .brkCmp = LIN_DEFAULT_BREAK_CMP,
    /* Sync field measurements */
    .syncStart = true,
    .syncBt = { 0 },
    .syncOkCnt = 0,
    .syncErrCnt = 0,
    /* Bit time calculations */
    .btSum = 0,
    .btAvg = 0,
    .btWidth = LIN_0_TBIT_WIDTH,
    .btWMax = LIN_0_TBIT_WIDTH * LIN_AUTO_BAUD_MAX / 100,
    .btWMin = LIN_0_TBIT_WIDTH * LIN_AUTO_BAUD_MIN / 100,
    /* Baud rate tracking */
    .brMeas = 0,
    .brPrev = LIN_0_BAUD_RATE,
    .brCurr = LIN_0_BAUD_RATE,
    /* Auto-baud state */
    .autoBaud = false,
    /* Response timing */
    .respDelay = LIN_RESPONSE_LAPSE,
    /* Timeout ticks */
    .timeoutTicks = LIN_TIMEOUT_TICKS_DEFAULT
};

/* LIN transmit/receive context with default values */
Lin_TxRxCtx_t gLIN = {
    .state = LIN_STATE_WAIT_FOR_BREAK,
    .respMsgTbl = responderMessageTable,
    .msgTblIdx = 0,
    .rxIdx = 0,
    .txIdx = 1,
    .rxBuf = { 0 },
    .txBuf = { 0 },
    .chkSum.word = 0,
    .currentPID = 0,
    .timing = &gLINT,
    .nodeState = LIN_NODE_STATE_OPERATIONAL,
    .lastError = LIN_ERROR_NO_ERROR,
    .errorCount = 0,
    .successCount = 0,
    .responseError = false,
    .busInactivityCounter = 0,
    .autoSleepEnabled = AUTO_SLEEP_ENABLED,
    .receivingDiagnosticFrame = false,
    .expectedDataLength = 8
};

uint8_t LIN_calculatePID(uint8_t frameID)
{
    uint8_t p0, p1;
    uint8_t id0 = (frameID >> 0) & 0x01;
    uint8_t id1 = (frameID >> 1) & 0x01;
    uint8_t id2 = (frameID >> 2) & 0x01;
    uint8_t id3 = (frameID >> 3) & 0x01;
    uint8_t id4 = (frameID >> 4) & 0x01;
    uint8_t id5 = (frameID >> 5) & 0x01;

    /* P0 = ID0 XOR ID1 XOR ID2 XOR ID4 */
    p0 = id0 ^ id1 ^ id2 ^ id4;

    /* P1 = NOT(ID1 XOR ID3 XOR ID4 XOR ID5) */
    p1 = ~(id1 ^ id3 ^ id4 ^ id5) & 0x01;

    return (frameID & 0x3F) | (p0 << 6) | (p1 << 7);
}

bool LIN_validatePID(uint8_t pid)
{
    uint8_t frameID = pid & 0x3F;
    uint8_t expectedPID = LIN_calculatePID(frameID);
    return (pid == expectedPID);
}

uint8_t LIN_getFrameIDFromPID(uint8_t pid)
{
    return pid & 0x3F;
}

uint8_t LIN_calculateChecksum(uint8_t *data, uint8_t length, uint8_t pid, LIN_CHECKSUM_TYPE type)
{
    LIN_word_t chksum;
    uint8_t i;

    chksum.word = 0;

    for (i = 0; i < length; i++)
    {
        chksum.word += data[i];
    }

    if (type == LIN_CHECKSUM_ENHANCED)
    {
        chksum.word += pid;
    }

    chksum.word = chksum.byte[0] + chksum.byte[1];
    chksum.word = chksum.byte[0] + chksum.byte[1];

    return (uint8_t)(0xFF - chksum.byte[0]);
}

void LIN_Responder_setError(Lin_TxRxCtx_t *lin, LIN_ERROR error)
{
    lin->lastError = error;
    lin->errorCount++;

    /* Notify application of error */
    LIN_processError(error);
}

void LIN_Responder_clearError(Lin_TxRxCtx_t *lin)
{
    lin->lastError = LIN_ERROR_NO_ERROR;
}

void LIN_switchCounterToSyncMode(LIN_Peripheral_t peripheral)
{
    DL_UART_enableLINSyncFieldValidationCounterControl(peripheral);
    DL_UART_disableLINCounterCompareMatch(LIN_0_INST);
    DL_UART_disableLINCountWhileLow(peripheral);
}

void LIN_switchCounterToBreakMode(LIN_Peripheral_t peripheral)
{
    DL_UART_enableLINCounterCompareMatch(peripheral);
    DL_UART_Extend_enableLINCountWhileLow(peripheral);
    DL_UART_Extend_enableLINCounterClearOnFallingEdge(peripheral);
    DL_UART_Extend_enableLINCounter(peripheral);
}

void LIN_Responder_resetState(LIN_Peripheral_t peripheral, void *timer, Lin_TxRxCtx_t *lin)
{
    /* Reset state machine */
    lin->state = LIN_STATE_WAIT_FOR_BREAK;
    lin->rxIdx = 0;
    lin->txIdx = 1;
    lin->chkSum.word = 0;

    /* Reset timing context */
    gLINT.brkW = 0;
    gLINT.syncOkCnt = 0;
    gLINT.syncErrCnt = 0;
    gLINT.btSum = 0;
    gLINT.syncStart = true;
    gLINT.autoBaud = false;
    gLINT.respDelay = LIN_0_INST_FREQUENCY / (2 * gLINT.brMeas);
    gLINT.btWidth = gLINT.btAvg;
    gLINT.btWMin = (uint16_t)(gLINT.btWidth * LIN_AUTO_BAUD_MIN / 100);
    gLINT.btWMax = (uint16_t)(gLINT.btWidth * LIN_AUTO_BAUD_MAX / 100);

    /* Flush RX FIFO */
    while (!DL_UART_isRXFIFOEmpty(peripheral))
    {
        DL_UART_Extend_receiveData(peripheral);
    }

    /* Disable all data/sync interrupts */
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_RX);
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_TX);
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);

    /* Reconfigure LIN counter for break detection */
    LIN_switchCounterToBreakMode(peripheral);

    if (LIN_Responder_isBusSleeping(lin))
    {
        DL_UART_setLINCounterCompareValue(peripheral, LIN_WAKEUP_COMPARE_VALUE);
    }
    else
    {
        DL_UART_setLINCounterCompareValue(peripheral, gLINT.brkCmp);
    }

    /* Re-enable break detection interrupt */
    DL_UART_Extend_clearInterruptStatus(peripheral, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
    DL_UART_Extend_enableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);

    /* Stop timeout timer */
    DL_Timer_stopCounter((GPTIMER_Regs *)timer);
}

void LIN_prepareForNextFrame(LIN_Peripheral_t peripheral, void *timer)
{
    LIN_switchCounterToBreakMode(peripheral);
    if (LIN_Responder_isBusSleeping(&gLIN))
    {
        DL_UART_setLINCounterCompareValue(peripheral, LIN_WAKEUP_COMPARE_VALUE);
    }
    else
    {
        DL_UART_setLINCounterCompareValue(peripheral, gLINT.brkCmp);
    }
    DL_UART_Extend_clearInterruptStatus(peripheral, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
    DL_UART_Extend_enableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
    DL_Timer_stopCounter((GPTIMER_Regs *)timer);
}

void LIN_Responder_sendWakeup(LIN_Peripheral_t peripheral)
{
    DL_UART_enableLINSendBreak(peripheral);
    delay_cycles(LIN_WAKEUP_PULSE_CYCLES);
    DL_UART_disableLINSendBreak(peripheral);

    gLIN.nodeState = LIN_NODE_STATE_OPERATIONAL;
}

void LIN_Responder_enterSleep(LIN_Peripheral_t peripheral, Lin_TxRxCtx_t *lin)
{
    lin->nodeState = LIN_NODE_STATE_BUS_SLEEP;
    lin->busInactivityCounter = 0;
    lin->responseError = false;
    LIN_processSleep();
    /* Lower the break detection threshold for wakeup detection */
    DL_UART_setLINCounterCompareValue(peripheral, LIN_WAKEUP_COMPARE_VALUE);
}

void LIN_Responder_wakeup(Lin_TxRxCtx_t *lin)
{
    lin->nodeState = LIN_NODE_STATE_OPERATIONAL;
    lin->busInactivityCounter = 0;
}

bool LIN_Responder_isBusSleeping(Lin_TxRxCtx_t *lin)
{
    return (lin->nodeState == LIN_NODE_STATE_BUS_SLEEP);
}

bool LIN_Responder_checkSleepCommand(uint8_t *data)
{
    return (data[0] == LIN_SLEEP_CMD_BYTE1);
}

void LIN_Responder_receivePID(LIN_Peripheral_t peripheral, void *timer, uint8_t rxByte, Lin_TxRxCtx_t *lin)
{
    uint8_t tableIndex;
    uint8_t frameID;

    /* Store current PID for checksum calculation */
    lin->currentPID = rxByte;

    /* Validate PID parity */
    if (!LIN_validatePID(rxByte))
    {
        LIN_Responder_setError(lin, LIN_ERROR_PID_PARITY);
        LIN_Responder_resetState(peripheral, timer, lin);
        return;
    }

    frameID = LIN_getFrameIDFromPID(rxByte);

    /* Search message table for matching PID */
    for (tableIndex = 0; tableIndex < LIN_RESPONDER_NUM_MSGS; tableIndex++)
    {
        if (lin->respMsgTbl[tableIndex].msgPID == rxByte || lin->respMsgTbl[tableIndex].msgID == frameID)
        {
            lin->msgTblIdx = tableIndex;
            break;
        }
    }

    /* No matching PID found */
    if (tableIndex >= LIN_RESPONDER_NUM_MSGS)
    {
        /* Check for diagnostic frame 0x3C (go-to-sleep command) */
        if (frameID == LIN_FRAME_ID_COMMANDER_REQ)
        {
            lin->receivingDiagnosticFrame = true;
            lin->expectedDataLength = 8;
            lin->state = LIN_STATE_DATA_FIELD;
            DL_Timer_setLoadValue((GPTIMER_Regs *)timer, gLINT.timeoutTicks);
            DL_Timer_startCounter((GPTIMER_Regs *)timer);
            return;
        }
        LIN_Responder_resetState(peripheral, timer, lin);
        return;
    }

    /* Normal frame from table - clear diagnostic flag */
    lin->receivingDiagnosticFrame = false;
    lin->busInactivityCounter = 0;

    /* Direction-based processing */
    switch (lin->respMsgTbl[tableIndex].direction)
    {
    case LIN_DIRECTION_SUBSCRIBE:
        /* Responder receives data from commander */
        lin->state = LIN_STATE_DATA_FIELD;
        DL_Timer_setLoadValue((GPTIMER_Regs *)timer, gLINT.timeoutTicks);
        DL_Timer_startCounter((GPTIMER_Regs *)timer);
        break;

    case LIN_DIRECTION_PUBLISH:
        /* Responder sends data to commander */
        DL_Timer_stopCounter((GPTIMER_Regs *)timer);
        DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_RX);

        /* Add delay to ensure STOP bit after PID is completely received */
        delay_cycles(gLINT.btWidth >> 1);

        /* TX via interrupt */
        lin->state = LIN_STATE_DATA_FIELD;
        DL_UART_Extend_clearInterruptStatus(peripheral, DL_UART_EXTEND_INTERRUPT_TX);
        DL_UART_Extend_enableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_TX);
        DL_UART_Extend_transmitData(peripheral, lin->txBuf[0]);
        break;

    default:
        DL_Timer_stopCounter((GPTIMER_Regs *)timer);
        LIN_Responder_resetState(peripheral, timer, lin);
        break;
    }
}

void LIN_Responder_receiveMessage(LIN_Peripheral_t peripheral, void *timer, uint8_t rxByte, Lin_TxRxCtx_t *lin)
{
    uint8_t checksum;
    uint8_t rxChecksum;
    LIN_function_ptr_t callbackFunction;
    LIN_CHECKSUM_TYPE checksumType;
    uint8_t frameID;
    uint8_t expectedSize;

    /* Determine expected size */
    expectedSize = lin->receivingDiagnosticFrame ? lin->expectedDataLength : lin->respMsgTbl[lin->msgTblIdx].msgSize;

    /* Receiving data bytes */
    if (lin->rxIdx < expectedSize)
    {
        lin->rxBuf[lin->rxIdx] = rxByte;
        lin->chkSum.word += lin->rxBuf[lin->rxIdx++];

        /* Restart timeout timer */
        DL_Timer_stopCounter((GPTIMER_Regs *)timer);
        DL_Timer_setLoadValue((GPTIMER_Regs *)timer, gLINT.timeoutTicks);
        DL_Timer_startCounter((GPTIMER_Regs *)timer);
        return;
    }

    /* Receiving checksum byte */
    rxChecksum = rxByte;
    DL_Timer_stopCounter((GPTIMER_Regs *)timer);
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_RX);

    /* Determine checksum type */
    checksumType = lin->respMsgTbl[lin->msgTblIdx].checksumType;

    /* Diagnostic frames always use CLASSIC checksum */
    frameID = LIN_getFrameIDFromPID(lin->currentPID);
    if (frameID == LIN_FRAME_ID_COMMANDER_REQ || frameID == LIN_FRAME_ID_RESPONDER_RESP)
    {
        checksumType = LIN_CHECKSUM_CLASSIC;
    }

    /* Calculate enhanced checksum */
    if (checksumType == LIN_CHECKSUM_ENHANCED)
    {
        lin->chkSum.word += lin->currentPID;
    }
    lin->chkSum.word = lin->chkSum.byte[0] + lin->chkSum.byte[1];
    checksum = lin->chkSum.byte[0];
    checksum += lin->chkSum.byte[1];
    checksum = 0xFF - checksum;

    /* Validate checksum */
    if (rxChecksum == checksum)
    {
        if (frameID == LIN_FRAME_ID_COMMANDER_REQ)
        {
            if (LIN_Responder_checkSleepCommand(lin->rxBuf))
            {
                LIN_Responder_enterSleep(peripheral, lin);
            }
        }

        /* Call callback function if present (only for table frames) */
        if (!lin->receivingDiagnosticFrame)
        {
            callbackFunction = lin->respMsgTbl[lin->msgTblIdx].callbackFunction;
            if (callbackFunction != NULL)
            {
                callbackFunction();
            }
        }
        lin->successCount++;
        LIN_Responder_clearError(lin);
    }
    else
    {
        LIN_Responder_setError(lin, LIN_ERROR_CHECKSUM);
    }

    /* Reset for next frame */
    lin->rxIdx = 0;
    lin->chkSum.word = 0;
    lin->state = LIN_STATE_WAIT_FOR_BREAK;
    lin->receivingDiagnosticFrame = false;

    LIN_prepareForNextFrame(peripheral, timer);
}

void LIN_Responder_transmitMessage(LIN_Peripheral_t peripheral, void *timer, Lin_TxRxCtx_t *lin)
{
    uint8_t checksum;

    /* More data bytes to send */
    if (lin->txIdx < lin->respMsgTbl[lin->msgTblIdx].msgSize)
    {
        lin->chkSum.word += lin->txBuf[lin->txIdx];
        DL_UART_Extend_transmitData(peripheral, lin->txBuf[lin->txIdx++]);
        return;
    }

    /* All data bytes sent - transmit checksum */
    if (lin->txIdx == lin->respMsgTbl[lin->msgTblIdx].msgSize)
    {
        LIN_CHECKSUM_TYPE checksumType;
        uint8_t frameID;

        /* Determine checksum type */
        checksumType = lin->respMsgTbl[lin->msgTblIdx].checksumType;

        /* Diagnostic frames always use CLASSIC checksum */
        frameID = LIN_getFrameIDFromPID(lin->currentPID);
        if (frameID == LIN_FRAME_ID_COMMANDER_REQ || frameID == LIN_FRAME_ID_RESPONDER_RESP)
        {
            checksumType = LIN_CHECKSUM_CLASSIC;
        }

        /* Calculate checksum - add first byte that wasn't accumulated in loop */
        lin->chkSum.word = lin->chkSum.word + lin->txBuf[0];

        /* Add PID for enhanced checksum */
        if (checksumType == LIN_CHECKSUM_ENHANCED)
        {
            lin->chkSum.word += lin->currentPID;
        }

        lin->chkSum.word = lin->chkSum.byte[0] + lin->chkSum.byte[1];
        checksum = lin->chkSum.byte[0];
        checksum += lin->chkSum.byte[1];
        checksum = 0xFF - checksum;

        DL_UART_Extend_transmitData(peripheral, checksum);
        lin->txIdx++;
        return;
    }

    /* Checksum sent - transmission complete */
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_TX);

    /* Wait for transmission to complete */
    while (DL_UART_Extend_isBusy(peripheral));

    /* Clear RX FIFO (TX echo) */
    while (!DL_UART_isRXFIFOEmpty(peripheral))
    {
        (void)DL_UART_Extend_receiveData(peripheral);
    }

    /* Call TX callback */
    LIN_processMessage_Tx();

    /* Reset state */
    lin->txIdx = 1;
    lin->chkSum.word = 0;
    lin->state = LIN_STATE_WAIT_FOR_BREAK;

    /* Prepare for next frame */
    LIN_prepareForNextFrame(peripheral, timer);
    lin->successCount++;
}

static const DL_UART_Extend_ClockConfig gLIN_0ClockConfig = {
    .clockSel    = DL_UART_EXTEND_CLOCK_BUSCLK,
    .divideRatio = DL_UART_EXTEND_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Extend_Config gLIN_0Config = {
    .mode        = DL_UART_EXTEND_MODE_NORMAL,
    .direction   = DL_UART_EXTEND_DIRECTION_TX_RX,
    .flowControl = DL_UART_EXTEND_FLOW_CONTROL_NONE,
    .parity      = DL_UART_EXTEND_PARITY_NONE,
    .wordLength  = DL_UART_EXTEND_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_EXTEND_STOP_BITS_ONE
};

void LIN_0_INST_IRQHandler(void)
{
    switch (DL_UART_Extend_getPendingInterrupt(LIN_0_INST))
    {
    case DL_UART_EXTEND_IIDX_LIN_FALLING_EDGE:
        if (gLIN.state == LIN_STATE_WAIT_FOR_BREAK)
        {
            /* Wake up if sleeping - any bus activity acts as wake-up */
            if (LIN_Responder_isBusSleeping(&gLIN))
            {
                LIN_Responder_wakeup(&gLIN);

                /* Restore normal break detection threshold */
                DL_UART_setLINCounterCompareValue(LIN_0_INST, gLINT.brkCmp);
                LIN_processWakeUp();
                gLIN.state = LIN_STATE_WAIT_FOR_BREAK;
                LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                break;
            }
            gLIN.state = LIN_STATE_BREAK_FIELD;
            /* Flush RX FIFO */
            while (!DL_UART_isRXFIFOEmpty(LIN_0_INST))
            {
                DL_UART_Extend_receiveData(LIN_0_INST);
            }

            /* Enable rising edge capture for break validation */
            DL_UART_enableLINRisingEdgeCapture(LIN_0_INST);
            DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
            DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
        }
        else if(gLIN.state == LIN_STATE_SYNC_FIELD_NEG_EDGE)
        {
            if(syncStartBit == 1)
            {
                syncStartBit = 0;
                break;
            }

            gLINT.syncBt[gLINT.syncOkCnt+gLINT.syncErrCnt].negEdge = DL_UART_Extend_getLINFallingEdgeCaptureValue(LIN_0_INST);

            /* Validation check of sync field segment timing */
            if (((gLINT.syncBt[gLINT.syncOkCnt+gLINT.syncErrCnt].negEdge >> 1) > gLINT.btWMin) && ((gLINT.syncBt[gLINT.syncOkCnt+gLINT.syncErrCnt].negEdge >> 1) < gLINT.btWMax))
            {
                gLINT.syncOkCnt++;
            }
            else
            {
                gLINT.btSum += gLINT.syncBt[gLINT.syncOkCnt+gLINT.syncErrCnt].negEdge;
                gLINT.syncErrCnt++;
            }

            /* All 4 sync cycles measured */
            if ((gLINT.syncErrCnt + gLINT.syncOkCnt) == LIN_RESPONDER_SYNC_CYCLES)
            {
                DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
                /* Sync OK - proceed to receive data */
                DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
                DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
                gLINT.brPrev = gLINT.brCurr;
                gLIN.state = LIN_STATE_SYNC_FIELD_POS_EDGE;
            }

            /* Check if auto-baud recalibration is needed */
            else if (gLINT.syncErrCnt >= AUTO_BAUD_THRESHOLD)
            {
                if(AUTO_BAUD_ENABLED)
                {
                    gLINT.autoBaud = true;
                }
                else
                {
                    LIN_Responder_setError(&gLIN, LIN_ERROR_SYNC);
                    LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                }
            }
        }
        break;

    case DL_UART_EXTEND_IIDX_LIN_RISING_EDGE:
        if (gLIN.state == LIN_STATE_BREAK_FIELD)
        {

            gLINT.brkW += DL_UART_Extend_getLINRisingEdgeCaptureValue(LIN_0_INST);

            /* Check minimum break length */
            if (AUTO_BAUD_ENABLED)
            {
                if (gLINT.brkW < LIN_BREAK_COMPARE_MIN)
                {
                    LIN_Responder_setError(&gLIN, LIN_ERROR_BREAK_SHORT);
                    LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                    break;
                }
            }
            else
            {
                if (gLINT.brkW < (uint32_t)(gLINT.btWidth * LIN_TBIT_BREAK_FIELD_MIN))
                {
                    LIN_Responder_setError(&gLIN, LIN_ERROR_BREAK_SHORT);
                    LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                    break;
                }
            }

            /* Validate break field length:
             * - If AUTO_BAUD: check against max based on min supported baud rate
             * - Otherwise: check against current baud rate's max break width
             */
            if (AUTO_BAUD_ENABLED)
            {
                if (gLINT.brkW >= LIN_BREAK_COMPARE_MAX)
                {
                    LIN_Responder_setError(&gLIN, LIN_ERROR_BREAK_LONG);
                    LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                    break;
                }
            }
            else
            {
                if (gLINT.brkW >= (uint32_t)(gLINT.btWidth * LIN_TBIT_BREAK_FIELD_MAX))
                {
                    LIN_Responder_setError(&gLIN, LIN_ERROR_BREAK_LONG);
                    LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                    break;
                }
            }

            /* Valid break field - prepare for sync field measurement */
            gLINT.brkW = 0;
            gLINT.syncOkCnt = 0;
            gLINT.syncErrCnt = 0;
            gLINT.btSum = 0;
            gLINT.syncStart = true;
            syncStartBit = 1;

            gLIN.state = LIN_STATE_SYNC_FIELD_NEG_EDGE;
            LIN_switchCounterToSyncMode(LIN_0_INST);

            DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
            DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
            DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
        }
        else if(gLIN.state == LIN_STATE_SYNC_FIELD_POS_EDGE)
        {
            /* Sync complete - prepare for RX data reception */
            DL_UART_Extend_disableLINCounter(LIN_0_INST);
            DL_UART_disableLINRisingEdgeCapture(LIN_0_INST);
            DL_UART_disableLINFallingEdgeCapture(LIN_0_INST);
            DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);

            if(gLINT.autoBaud == false)
            {
                rxByte = DL_UART_Extend_receiveDataBlocking(LIN_0_INST);
                /* SYNC byte validation */
                if (rxByte != LIN_SYNC_BYTE)
                {
                    LIN_Responder_setError(&gLIN, LIN_ERROR_SYNC);
                    LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                    break;
                }
            }
            else
            {
                /* Auto-baud recalibration */
                gLINT.btAvg = (gLINT.btSum >> 1) / gLINT.syncErrCnt;
                gLINT.brMeas = LIN_0_INST_FREQUENCY / gLINT.btAvg;
                gLINT.timeoutTicks = LIN_CALC_TIMEOUT_TICKS(gLINT.brMeas);
                gLINT.brPrev = gLINT.brCurr;
                gLINT.brCurr = gLINT.brMeas;

                if((gLINT.brMeas < LIN_MIN_SUPPORTED_BAUD) || (gLINT.brMeas > LIN_MAX_SUPPORTED_BAUD))
                {
                    LIN_Responder_setError(&gLIN, LIN_ERROR_SYNC);
                    LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
                    break;
                }

                DL_UART_Extend_disable(LIN_0_INST);
                DL_UART_Extend_disablePower(LIN_0_INST);
                DL_UART_Extend_reset(LIN_0_INST);
                DL_UART_Extend_enablePower(LIN_0_INST);
                delay_cycles(POWER_STARTUP_DELAY);
                DL_UART_Extend_setClockConfig(LIN_0_INST, (DL_UART_Extend_ClockConfig *) &gLIN_0ClockConfig);
                DL_UART_Extend_init(LIN_0_INST, (DL_UART_Extend_Config *) &gLIN_0Config);
                DL_UART_Extend_setOversampling(LIN_0_INST, DL_UART_OVERSAMPLING_RATE_16X);
                DL_UART_Extend_configBaudRate(LIN_0_INST, LIN_0_INST_FREQUENCY,gLINT.brMeas);
                DL_UART_Extend_enable(LIN_0_INST);
                gLINT.autoBaud = true;
            }
            gLIN.state = LIN_STATE_PID_FIELD;

            DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);
            DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);

            /* Configure timeout for RX */
            DL_Timer_setLoadValue(TIMER_0_INST, gLINT.timeoutTicks);
            DL_Timer_setTimerCount(TIMER_0_INST, gLINT.timeoutTicks);
            DL_Timer_startCounter(TIMER_0_INST);
        }

        break;

    case DL_UART_EXTEND_IIDX_LIN_COUNTER_OVERFLOW:
        if (gLIN.state == LIN_STATE_BREAK_FIELD)
        {
            gLINT.brkW += LIN_MAX_CNTR_VAL;
            if (gLINT.brkW > (LIN_MAX_CNTR_VAL * LIN_MAX_BREAK_MULTIPLIER))
            {
                LIN_Responder_setError(&gLIN, LIN_ERROR_BREAK_LONG);
                LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
            }
        }
        else
        {
            /* Counter overflow during sync */
            LIN_Responder_setError(&gLIN, LIN_ERROR_SYNC);
            LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
        }
        break;

    case DL_UART_EXTEND_IIDX_RX:
        rxByte = DL_UART_Extend_receiveData(LIN_0_INST);
        DL_Timer_setTimerCount(TIMER_0_INST, gLINT.timeoutTicks);
        if (gLIN.state == LIN_STATE_PID_FIELD)
        {
            LIN_Responder_receivePID(LIN_0_INST, TIMER_0_INST, rxByte, &gLIN);
        }
        else if (gLIN.state == LIN_STATE_DATA_FIELD)
        {
            LIN_Responder_receiveMessage(LIN_0_INST, TIMER_0_INST, rxByte, &gLIN);
        }
        else
        {
            LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
        }
        break;

    case DL_UART_EXTEND_IIDX_TX:
        LIN_Responder_transmitMessage(LIN_0_INST, TIMER_0_INST, &gLIN);
        break;

    default:
        break;
    }
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_Timer_getPendingInterrupt(TIMER_0_INST))
    {
    case DL_TIMER_IIDX_ZERO:
        /* Timeout occurred - clean exit from any state */
        if (gLIN.state != LIN_STATE_WAIT_FOR_BREAK)
        {
            /* Set appropriate error based on bytes received */
            if (gLIN.rxIdx > 0)
            {
                LIN_Responder_setError(&gLIN, LIN_ERROR_INCOMPLETE_RESPONSE);
            }
            else
            {
                LIN_Responder_setError(&gLIN, LIN_ERROR_NO_RESPONSE);
            }
            LIN_Responder_resetState(LIN_0_INST, TIMER_0_INST, &gLIN);
        }
        break;

    default:
        break;
    }
}
