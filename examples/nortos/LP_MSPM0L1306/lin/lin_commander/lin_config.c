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

/* LIN commander context with default values */
Lin_TxRxCtx_t gLIN = {
    .rxState = LIN_RX_STATE_DATA,
    .state = LIN_STATE_WAIT_FOR_BREAK,
    .cmdMsgTbl = commanderMessageTable,
    .msgTblIdx = 0,
    .rxIdx = 0,
    .txIdx = 1,
    .rxBuf = { 0 },
    .txBuf = { 0 },
    .chkSum.word = 0,
    .txChkSum.word = 0,
    .currentPID = 0,
    .nodeState = LIN_NODE_STATE_OPERATIONAL,
    .lastError = LIN_ERROR_NO_ERROR,
    .errorCount = 0,
    .successCount = 0,
    .transactionCompleted = false
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

void LIN_Commander_setError(Lin_TxRxCtx_t *lin, LIN_ERROR error)
{
    lin->lastError = error;
    lin->errorCount++;

    /* Notify application of error */
    LIN_processError(error);
}

void LIN_Commander_clearError(Lin_TxRxCtx_t *lin)
{
    lin->lastError = LIN_ERROR_NO_ERROR;
}

void LIN_Commander_sendGoToSleep(LIN_Peripheral_t peripheral)
{
    uint8_t sleepData[8];
    uint8_t checksum;
    uint8_t i;
    uint8_t pid;

    gLIN.transactionCompleted = false;

    /* Prepare sleep command data: 0x00, 0xFF, 0xFF, ... */
    sleepData[0] = LIN_SLEEP_CMD_BYTE1;
    for (i = 1; i < 8; i++)
    {
        sleepData[i] = LIN_SLEEP_CMD_FILL;
    }

    /* Calculate PID for COMMANDER Request frame (0x3C) */
    pid = LIN_calculatePID(LIN_FRAME_ID_COMMANDER_REQ);

    /* Transmit break field */
    DL_UART_enableLINSendBreak(peripheral);
    delay_cycles(LIN_BREAK_LENGTH);
    DL_UART_disableLINSendBreak(peripheral);

    /* Transmit sync byte and PID */
    DL_UART_Extend_transmitDataBlocking(peripheral, LIN_SYNC_BYTE);
    DL_UART_Extend_transmitDataBlocking(peripheral, pid);

    /* Wait for PID transmission to complete */
    while (DL_UART_isBusy(peripheral));

    /* Transmit sleep command data */
    for (i = 0; i < 8; i++)
    {
        DL_UART_Extend_transmitDataBlocking(peripheral, sleepData[i]);
    }

    /* Calculate and transmit CLASSIC checksum (diagnostic frames use CLASSIC) */
    checksum = LIN_calculateChecksum(sleepData, 8, pid, LIN_CHECKSUM_CLASSIC);
    DL_UART_Extend_transmitDataBlocking(peripheral, checksum);

    /* Wait for transmission to complete */
    while (DL_UART_isBusy(peripheral));

    /* Flush RX FIFO (TX echo) */
    while (!DL_UART_isRXFIFOEmpty(peripheral))
    {
        (void)DL_UART_Extend_receiveData(peripheral);
    }

    /* Update node state */
    gLIN.nodeState = LIN_NODE_STATE_BUS_SLEEP;
    gLIN.transactionCompleted = true;
}

void LIN_Commander_sendWakeup(LIN_Peripheral_t peripheral)
{

    gLIN.transactionCompleted = false;
    /* Send wake-up signal (dominant pulse 250us - 5ms) */
    DL_UART_enableLINSendBreak(peripheral);
    delay_cycles(LIN_WAKEUP_PULSE_CYCLES);
    DL_UART_disableLINSendBreak(peripheral);

    /* Update node state */
    gLIN.nodeState = LIN_NODE_STATE_OPERATIONAL;
    gLIN.transactionCompleted = true;
}

bool LIN_Commander_isBusSleeping(Lin_TxRxCtx_t *lin)
{
    return (lin->nodeState == LIN_NODE_STATE_BUS_SLEEP);
}

/**
 * @brief Send LIN frame header and optional data
 */
void LIN_Commander_sendFrame(LIN_Peripheral_t peripheral, void *timer, uint8_t tableIndex, uint8_t *txBuffer, LIN_table_record_t *messageTable)
{
    /* Disable RX interrupt during header transmission */
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_RX);

    /* Store table index and PID for subsequent data handling */
    gLIN.msgTblIdx = tableIndex;
    gLIN.currentPID = messageTable[tableIndex].msgPID;
    gLIN.transactionCompleted = false;

    /*
     * Transmit LIN frame header:
     *   1. Break field (dominant for >= 13 Tbit)
     *   2. Sync byte (0x55)
     *   3. Protected Identifier (PID)
     */
    DL_UART_enableLINSendBreak(peripheral);
    delay_cycles(LIN_BREAK_LENGTH);
    DL_UART_disableLINSendBreak(peripheral);

    DL_UART_Extend_transmitDataBlocking(peripheral, LIN_SYNC_BYTE);
    DL_UART_Extend_transmitDataBlocking(peripheral, messageTable[tableIndex].msgPID);

    /* Wait for PID transmission to complete */
    while (DL_UART_isBusy(peripheral));

    /* Flush RX FIFO to clear echo data */
    while (!DL_UART_isRXFIFOEmpty(peripheral))
    {
        DL_UART_Extend_receiveData(peripheral);
    }

    /*
     * Direction-based processing:
     *   - PUBLISH: Commander sends data
     *   - SUBSCRIBE: Commander receives data (callback present)
     *   - NONE: Header only (RESPONDER-to-RESPONDER)
     */
    switch (messageTable[tableIndex].direction)
    {
    case LIN_DIRECTION_PUBLISH:
        /* Transmit mode: Commander sends data to responder */
        gLIN.txIdx = 1;
        gLIN.txChkSum.word = 0;

        /* TX interrupt mode: Send first byte, ISR handles rest */
        if(messageTable[tableIndex].msgSize >= 1)
        {
            DL_UART_Extend_clearInterruptStatus(peripheral, DL_UART_EXTEND_INTERRUPT_TX);
            DL_UART_Extend_enableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_TX);
            DL_UART_Extend_transmitData(peripheral, txBuffer[0]);
        }
        else
        {
            gLIN.transactionCompleted = true;
        }
        break;

    case LIN_DIRECTION_SUBSCRIBE:
        /* Receive mode: Prepare to receive response data from responder */
        gLIN.rxIdx = 0;
        gLIN.chkSum.word = 0;
        gLIN.rxState = LIN_RX_STATE_DATA;

        /* Flush RX buffer and clear pending interrupts */
        while (!DL_UART_isRXFIFOEmpty(peripheral))
        {
            DL_UART_Extend_receiveData(peripheral);
        }
        DL_UART_Extend_clearInterruptStatus(peripheral, DL_UART_EXTEND_INTERRUPT_RX);
        DL_UART_Extend_enableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_RX);

        /* Start frame timeout timer */
        DL_Timer_setLoadValue((GPTIMER_Regs *)timer, TIMEOUT);
        DL_Timer_startCounter((GPTIMER_Regs *)timer);
        break;

    default:
        gLIN.transactionCompleted = true;
        break;
    }
}

/**
 * @brief Process received response data byte
 */
void LIN_Commander_receiveMessage(LIN_Peripheral_t peripheral, void *timer, uint8_t rxByte, uint8_t *rxBuffer, LIN_table_record_t *messageTable)
{
    uint8_t checksum;
    uint8_t rxChecksum;
    LIN_function_ptr_t callbackFunction;
    LIN_CHECKSUM_TYPE checksumType;
    uint8_t frameID;

    switch (gLIN.rxState)
    {
    /*
     * DATA state: Accumulate data bytes
     */
    case LIN_RX_STATE_DATA:
        /* Store byte in receive buffer */
        rxBuffer[gLIN.rxIdx] = rxByte;

        /* Accumulate checksum (16-bit to handle carry) */
        gLIN.chkSum.word += rxBuffer[gLIN.rxIdx];

        /* Check if all data bytes received */
        gLIN.rxIdx++;
        if (gLIN.rxIdx >= messageTable[gLIN.msgTblIdx].msgSize)
        {
            /* Transition to checksum state */
            gLIN.rxState = LIN_RX_STATE_CHECKSUM;
        }

        /* Restart timeout timer for next byte */
        DL_Timer_stopCounter((GPTIMER_Regs *)timer);
        DL_Timer_setLoadValue((GPTIMER_Regs *)timer, TIMEOUT);
        DL_Timer_startCounter((GPTIMER_Regs *)timer);
        break;

    /*
     * CHECKSUM state: Validate frame checksum
     */
    case LIN_RX_STATE_CHECKSUM:
        rxChecksum = rxByte;

        /* Stop timeout timer */
        DL_Timer_stopCounter((GPTIMER_Regs *)timer);

        /* Disable RX until next frame */
        DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_RX);

        /* Determine checksum type */
        checksumType = messageTable[gLIN.msgTblIdx].checksumType;

        /* Diagnostic frames always use CLASSIC checksum */
        frameID = LIN_getFrameIDFromPID(gLIN.currentPID);
        if (frameID == LIN_FRAME_ID_COMMANDER_REQ || frameID == LIN_FRAME_ID_RESPONDER_RESP)
        {
            checksumType = LIN_CHECKSUM_CLASSIC;
        }

        /*
         * Enhanced checksum calculation:
         *   1. Add PID to running sum (if enhanced)
         *   2. Fold carry into lower byte
         *   3. Invert result
         */
        if (checksumType == LIN_CHECKSUM_ENHANCED)
        {
            gLIN.chkSum.word += gLIN.currentPID;
        }
        gLIN.chkSum.word = gLIN.chkSum.byte[0] + gLIN.chkSum.byte[1];
        checksum = gLIN.chkSum.byte[0];
        checksum += gLIN.chkSum.byte[1];
        checksum = 0xFF - checksum;

        /* Reset state for next frame */
        gLIN.rxIdx = 0;
        gLIN.chkSum.word = 0;
        gLIN.rxState = LIN_RX_STATE_DATA;
        gLIN.transactionCompleted = true;

        /* Validate checksum and invoke callback */
        if (rxChecksum == checksum)
        {
            callbackFunction = messageTable[gLIN.msgTblIdx].callbackFunction;
            if (callbackFunction != NULL)
            {
                callbackFunction();
            }
            gLIN.successCount++;
            LIN_Commander_clearError(&gLIN);
        }
        else
        {
            LIN_Commander_setError(&gLIN, LIN_ERROR_CHECKSUM);
        }
        break;

    default:
        /* Reset to data state on unexpected condition */
        gLIN.rxState = LIN_RX_STATE_DATA;
        gLIN.transactionCompleted = true;
        break;
    }
}

/**
 * @brief Transmit data bytes and checksum (TX interrupt mode)
 */
void LIN_Commander_transmitMessage(LIN_Peripheral_t peripheral, uint8_t *txBuffer, LIN_table_record_t *messageTable)
{

    uint8_t checksum;
    LIN_CHECKSUM_TYPE checksumType;
    uint8_t frameID;

    /* More data bytes to send */
    if (gLIN.txIdx < messageTable[gLIN.msgTblIdx].msgSize)
    {
        delay_cycles(LIN_0_TBIT_WIDTH*2);
        gLIN.txChkSum.word += txBuffer[gLIN.txIdx];
        DL_UART_Extend_transmitData(peripheral, txBuffer[gLIN.txIdx++]);
        return;
    }

    /* All data bytes sent - transmit checksum */
    if (gLIN.txIdx == messageTable[gLIN.msgTblIdx].msgSize)
    {
        /* Determine checksum type */
        checksumType = messageTable[gLIN.msgTblIdx].checksumType;

        /* Diagnostic frames always use CLASSIC checksum */
        frameID = LIN_getFrameIDFromPID(gLIN.currentPID);
        if (frameID == LIN_FRAME_ID_COMMANDER_REQ || frameID == LIN_FRAME_ID_RESPONDER_RESP)
        {
            checksumType = LIN_CHECKSUM_CLASSIC;
        }

        /* Calculate checksum */
        gLIN.txChkSum.word = gLIN.txChkSum.word + txBuffer[0];
        if (checksumType == LIN_CHECKSUM_ENHANCED)
        {
            gLIN.txChkSum.word += gLIN.currentPID;
        }
        gLIN.txChkSum.word = gLIN.txChkSum.byte[0] + gLIN.txChkSum.byte[1];
        checksum = gLIN.txChkSum.byte[0];
        checksum += gLIN.txChkSum.byte[1];
        checksum = 0xFF - checksum;

        gLIN.txIdx++;
        DL_UART_Extend_transmitData(peripheral, checksum);
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
    gLIN.successCount++;

    /* Reset state */
    gLIN.txIdx = 1;
    gLIN.txChkSum.word = 0;
    gLIN.transactionCompleted = true;
}

/**
 * @brief Handle RX timeout
 */
void LIN_Commander_handleTimeout(LIN_Peripheral_t peripheral, void *timer)
{
    /* Disable RX interrupt */
    DL_UART_Extend_disableInterrupt(peripheral, DL_UART_EXTEND_INTERRUPT_RX);

    /* Stop timeout timer */
    DL_Timer_stopCounter((GPTIMER_Regs *)timer);

    /* Set appropriate error based on bytes received */
    if (gLIN.rxIdx > 0)
    {
        LIN_Commander_setError(&gLIN, LIN_ERROR_INCOMPLETE_RESPONSE);
    }
    else
    {
        LIN_Commander_setError(&gLIN, LIN_ERROR_NO_RESPONSE);
    }

    /* Reset state for next frame */
    gLIN.rxIdx = 0;
    gLIN.chkSum.word = 0;
    gLIN.rxState = LIN_RX_STATE_DATA;
    gLIN.transactionCompleted = true;
}

void LIN_0_INST_IRQHandler(void)
{
    uint8_t rxByte;

    switch (DL_UART_Extend_getPendingInterrupt(LIN_0_INST))
    {
    /*
     * RX DATA HANDLING - Receive response data from responder
     */
    case DL_UART_EXTEND_IIDX_RX:
        rxByte = DL_UART_Extend_receiveData(LIN_0_INST);
        LIN_Commander_receiveMessage(LIN_0_INST, TIMER_0_INST, rxByte, gLIN.rxBuf, gLIN.cmdMsgTbl);
        break;

    /*
     * TX INTERRUPT - Transmit next data byte
     */
    case DL_UART_EXTEND_IIDX_TX:
        LIN_Commander_transmitMessage(LIN_0_INST, gLIN.txBuf, gLIN.cmdMsgTbl);
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
        /* Handle RX timeout */
        LIN_Commander_handleTimeout(LIN_0_INST, TIMER_0_INST);
        break;

    default:
        break;
    }
}
