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

/* Include LIN configuration, type definitions, and API declarations */
#include "lin_config.h"

/* Global LIN timing context initialized with compile-time defaults derived from SysConfig values */
volatile Lin_timing_t gLINT = {
    /* Break field timing */
    /* Initial measured break width; set to 0 until first break is received */
    .brkW = 0,
    /* Initial counter compare set to minimum valid break length at maximum supported baud rate */
    .brkCmp = LIN_BREAK_COMPARE_MIN_CYCLES,
    /* Sync field measurements */
    /* syncStart true indicates the state machine expects the first SYNC edge next */
    .syncStart = true,
    /* Zero-initialize all captured SYNC edge timing pairs */
    .syncBt = { 0 },
    /* No valid SYNC cycles measured yet */
    .syncOkCnt = 0,
    /* No failed SYNC cycles measured yet */
    .syncErrCnt = 0,
    /* Bit time calculations */
    /* Accumulated bit time sum starts at zero before any SYNC measurement */
    .btSum = 0,
    /* Average bit time starts at zero before first calculation */
    .btAvg = 0,
    /* Current bit time set to the nominal value from SysConfig */
    .btWidth = LIN_0_TBIT_WIDTH,
    /* Reference bit time preserved for reset and break validation, set to nominal SysConfig value */
    .intialBtWidth = LIN_0_TBIT_WIDTH,
    /* Bit time limit corresponding to 20 kbps (1/20000 s * LIN_0_INST_FREQUENCY) */
    .btWidthLimit = 2083U,
    /* Maximum acceptable bit time: nominal + 5% */
    .btWMax = LIN_0_TBIT_WIDTH * LIN_AUTO_BAUD_MAX / 100,
    /* Minimum acceptable bit time: nominal - 5% */
    .btWMin = LIN_0_TBIT_WIDTH * LIN_AUTO_BAUD_MIN / 100,
    /* Baud rate tracking */
    /* Measured baud rate starts at 0 until first SYNC field is processed */
    .brMeas = 0,
    /* Previous baud rate initialized to the SysConfig nominal baud rate */
    .brPrev = LIN_0_BAUD_RATE,
    /* Current baud rate initialized to the SysConfig nominal baud rate */
    .brCurr = LIN_0_BAUD_RATE,
    /* Auto-baud state */
    /* Auto-baud calibration is not active at startup */
    .autoBaud = false,
    /* Response timing */
    /* Response lapse: half-bit delay between PID stop bit and first data byte start bit */
    .respDelay = LIN_RESPONSE_LAPSE,
    /* Timeout ticks */
    /* Default inter-byte timeout in LFCLK ticks computed from nominal baud rate */
    .timeoutTicks = LIN_TIMEOUT_TICKS_DEFAULT,
    /* First SYNC validation has not yet completed */
    .firstSyncDone = false
};

/* Global LIN TX/RX context initialized with default values for a freshly powered node */
Lin_TxRxCtx_t gLIN = {
    /* Start in the initial state waiting for a valid break field */
    .state = LIN_RX_STATE_WAIT_FOR_BREAK,
    /* No message table entry has been matched yet */
    .msgTblIdx = 0,
    /* No PID is being processed at startup */
    .currentPID = 0,
    /* Link TX/RX context to the global timing structure */
    .timing = &gLINT,
    /* Channel starts idle: no frame in progress */
    .channelStatus = LIN_CHANNEL_IDLE,
    /* Network starts operational: not in sleep mode */
    .networkStatus = LIN_CHANNEL_OPERATIONAL,
    /* No error has occurred at startup */
    .lastError = LIN_ERROR_NO_ERROR,
    /* Zero error count at startup */
    .errorCount = 0,
    /* Zero success count at startup */
    .successCount = 0,
    /* No response error pending at startup */
    .responseError = false,
    /* Not receiving a diagnostic frame at startup */
    .receivingDiagnosticFrame = false,
    /* Default expected data length is maximum (8 bytes) */
    .expectedDataLength = 8,
    /* Channel status starts as sleep per LIN spec (node wakes on first wakeup or frame) */
    .status = LIN_CH_SLEEP,
    /* Apply SYNC tolerance check to every frame by default */
    .syncCheckType = LIN_SYNC_CHECK_ALL_FRAMES
};

/* LIN_calculatePID: computes the 8-bit Protected Identifier from a 6-bit frame ID
 * by appending two parity bits P0 and P1 as defined in the LIN 2.x specification */
uint8_t LIN_calculatePID(uint8_t frameID)
{
    uint8_t p0, p1;
    /* Extract individual ID bits 0 through 5 for parity calculation */
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

    /* Assemble PID: lower 6 bits = frame ID, bit 6 = P0, bit 7 = P1 */
    return (frameID & 0x3F) | (p0 << 6) | (p1 << 7);
}

/* LIN_validatePID: verifies that the parity bits embedded in the received PID
 * match those computed from the 6-bit frame ID; returns true if valid */
bool LIN_validatePID(uint8_t pid)
{
    /* Strip parity bits to recover the raw 6-bit frame ID */
    uint8_t frameID = pid & 0x3F;
    /* Recompute expected PID from the frame ID */
    uint8_t expectedPID = LIN_calculatePID(frameID);
    /* Return true only if received PID exactly matches the recomputed PID */
    return (pid == expectedPID);
}

/* LIN_SetError: records the error code, increments the error counter,
 * and invokes the application error callback */
void LIN_SetError(Lin_TxRxCtx_t *lin, LIN_ERROR error)
{
    /* Store the error type for later retrieval by the application */
    lin->lastError = error;
    /* Increment cumulative error counter */
    lin->errorCount++;

    /* Notify application of error */
    LIN_processError(error);
}

/* LIN_Responder_clearError: resets the last error field to LIN_ERROR_NO_ERROR
 * without modifying the error counter */
void LIN_Responder_clearError(Lin_TxRxCtx_t *lin)
{
    lin->lastError = LIN_ERROR_NO_ERROR;
}

/* Lin_FetchChannelStatus: returns the current Lin_StatusType of the global LIN context */
Lin_StatusType Lin_FetchChannelStatus()
{
    return gLIN.status;
}

/* Lin_enableFeaturesForSyncCheck: reconfigures the LIN counter and interrupt sources
 * to capture SYNC field falling and rising edges for bit-time measurement */
void Lin_enableFeaturesForSyncCheck()
{
    /* Disable compare-match interrupt while reconfiguring counter mode */
    DL_UART_disableLINCounterCompareMatch(LIN_0_INST);
    /* Stop counting while bus is low; counter will free-run for edge capture */
    DL_UART_disableLINCountWhileLow(LIN_0_INST);
    /* Arm falling edge interrupt for SYNC bit measurement */
    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
    /* Arm rising edge interrupt for SYNC bit measurement */
    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_POS_EDGE);
    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_POS_EDGE);
    /* Disable falling edge (timeout) interrupt; edge capture interrupts replace it */
    DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
    /* Enable the LIN sync field validation counter to capture edge timestamps */
    DL_UART_enableLINSyncFieldValidationCounterControl(LIN_0_INST);
    /* Disable raw RX data interrupt; data bytes are not expected during SYNC measurement */
    DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);

    /* Set the timeout compare value to detect a missing SYNC field */
    DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_TIMEOUT_DURATION);
    /* Mark that inter-byte timeout monitoring is now active */
    gLIN.timeoutEnabledForInterByte = true;
    /* Enable compare-match interrupt to detect SYNC timeout */
    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
    /* Clear any stale falling-edge flag before re-enabling the counter */
    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
    /* Reset counter to zero before starting the SYNC measurement window */
    DL_UART_setLINCounterValue(LIN_0_INST, 0);
    /* Start the counter */
    DL_UART_enableLINCounter(LIN_0_INST);
    /* Re-enable falling-edge interrupt to detect timeout via compare match */
    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
}

/* LIN_ResetState: stops and clears the LIN counter, sets the compare value to the
 * initial bit-time width, and re-enables the falling-edge interrupt to
 * detect the start of the next break field */
void LIN_ResetState(LIN_Peripheral_t peripheral, Lin_TxRxCtx_t *lin)
{
    /* Stop the LIN counter before modifying its configuration */
    DL_UART_disableLINCounter(LIN_0_INST);
    /* Disable counter-clear-on-falling-edge to prevent spurious resets */
    DL_UART_disableLINCounterClearOnFallingEdge(LIN_0_INST);
    /* Disable count-while-low since we are returning to break-detection mode */
    DL_UART_disableLINCountWhileLow(LIN_0_INST);
    /* Zero the counter for a clean start */
    DL_UART_setLINCounterValue(LIN_0_INST, 0);
    
    /* Re-enable compare match so the timeout fires if no valid break arrives */
    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);

    /* Set compare value to initial bit time width to define break detection window */
    DL_UART_setLINCounterCompareValue(LIN_0_INST, gLINT.intialBtWidth);

    /* Mask falling-edge interrupt before clearing flags to prevent re-entry */
    DL_UART_disableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_LIN_FALLING_EDGE);

    /* Clear all pending interrupt flags that may have accumulated during the error */
    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_INTERRUPT_BREAK_ERROR |
                                             DL_UART_INTERRUPT_LIN_FALLING_EDGE |
                                             DL_UART_INTERRUPT_LIN_COUNTER_OVERFLOW |
                                             DL_UART_INTERRUPT_RXD_POS_EDGE);

    /* Re-enable falling-edge interrupt to detect the next break field */
    DL_UART_enableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_LIN_FALLING_EDGE);

    /* Restart the counter to begin monitoring for the next frame */
    DL_UART_enableLINCounter(LIN_0_INST);
}

/* LIN_SendGoToSleepSignal: transmits the LIN master request go-to-sleep frame
 * (PID 0x3C, first data byte 0x00, remaining bytes 0xFF, classic checksum 0x00)
 * by directly driving break, sync, PID, data, and checksum bytes */
void LIN_SendGoToSleepSignal()
{
    /* Data bytes per LIN spec: first byte 0x00 signals sleep, remaining bytes 0xFF */
    uint8_t Lin_GoToSleepSdu_Ptr[8] = {0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU}; 
    /* Classic checksum for this fixed frame is 0x00 */
    uint8_t Lin_GoToSleepSdu_Checksum = 0x00U;
    
    /* Build the PDU descriptor for the go-to-sleep frame */
    Lin_PduType Lin_GoToSleep_Pdu =
    {
        /* Diagnostic master request PID (0x3C with parity = 0x3C) */
        .Pid = 0x3CU,
        /* Classic checksum excludes PID from calculation */
        .Cs = LIN_CLASSIC_CS,
        /* 8 data bytes */
        .Dl = 0x8U,
        /* Point to the sleep data buffer */
        .SduPtr = Lin_GoToSleepSdu_Ptr,
    };
    /* Block until TX FIFO has space to accept new data */
    while (DL_UART_isTXFIFOFull(LIN_0_INST) == true) 
    {
        /* Empty wait loop */
    };
    /* Transmit BREAK, SYNC byte, and PID */
    /* Assert break: pull bus dominant for the required break bit count */
    DL_UART_enableLINSendBreak(LIN_0_INST);
    /* Hold break for LIN_BREAK_BIT_COUNT bit times scaled to CPU clock cycles */
    delay_cycles(LIN_BREAK_BIT_COUNT * gLINT.btWidth * (CPUCLK_FREQ / LIN_0_INST_FREQUENCY));
    /* Release break: return bus to recessive */
    DL_UART_disableLINSendBreak(LIN_0_INST);
    /* Transmit the SYNC byte (0x55) for baud rate reference */
    DL_UART_transmitData(LIN_0_INST, LIN_SYNC_BYTE);
    /* Transmit the PID byte of the go-to-sleep frame */
    DL_UART_transmitData(LIN_0_INST, Lin_GoToSleep_Pdu.Pid);

    /* Transmit data */
    /* Transmit each data byte of the go-to-sleep payload sequentially */
    for (uint8_t locIndex = (uint8_t)0; locIndex < Lin_GoToSleep_Pdu.Dl; locIndex++) 
    {
        DL_UART_transmitData(LIN_0_INST, Lin_GoToSleep_Pdu.SduPtr[locIndex]);
    }

    /* Transmit Checksum */
    /* Transmit the pre-computed classic checksum byte (0x00 for this frame) */
    DL_UART_transmitData(LIN_0_INST, Lin_GoToSleepSdu_Checksum);
}

/* LIN_TransitionToSleepState: sets the network and channel status to sleep and
 * reconfigures the peripheral for the appropriate node type */
void LIN_TransitionToSleepState(Lin_TxRxCtx_t *lin)
{
    /* Mark network as sleeping */
    lin->networkStatus = LIN_CHANNEL_SLEEP;
    /* Mark channel status as sleep */
    lin->status = LIN_CH_SLEEP;
    /* For a responder node, reconfigure and disable all active interrupts */
    if(lin->node == LIN_RESPONDER)
    {
        /* Reconfigure peripheral to responder idle state */
        Lin_ConfigResponder();
        /* Return state machine to break-detection state */
        lin->state = LIN_RX_STATE_WAIT_FOR_BREAK;
        /* Disable all interrupt sources to prevent spurious wakeups during sleep */
        DL_UART_disableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_BREAK_ERROR |
                                             DL_UART_INTERRUPT_LIN_FALLING_EDGE |
                                             DL_UART_INTERRUPT_LIN_COUNTER_OVERFLOW |
                                             DL_UART_INTERRUPT_RXD_POS_EDGE |
                                             DL_UART_INTERRUPT_RX);
    }
    /* For a commander node, reconfigure and return to idle transmit state */
    else if (lin->node == LIN_COMMANDER)
    {
        /* Reconfigure peripheral to commander idle state */
        Lin_ConfigCommander();
        /* Return state machine to wait-for-send-frame state */
        lin->state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
    }
}

/* Lin_SendWakeupSignal: transmits a LIN wakeup dominant pulse of duration
 * corresponding to LIN_BREAK_BIT_COUNT bit times at the current baud rate */
void Lin_SendWakeupSignal()
{
    /* Assert dominant (low) state on the bus to generate the wakeup pulse */
    DL_UART_enableLINSendBreak(LIN_0_INST);
    /* Hold the wakeup pulse for the required duration in CPU cycles */
    delay_cycles(LIN_BREAK_BIT_COUNT * gLINT.btWidth * (CPUCLK_FREQ / LIN_0_INST_FREQUENCY));
    /* Release bus back to recessive after wakeup pulse */
    DL_UART_disableLINSendBreak(LIN_0_INST);
}

/* Lin_CheckWakeupStatus: returns the value of the external wakeup flag,
 * which is set by the ISR when a valid wakeup pulse is detected */
bool Lin_CheckWakeupStatus()
{
    return gLIN.extWakeUpStatus;
}

/* Lin_TransitionToWakeUpState: restores the channel to LIN_OPERATIONAL and
 * reconfigures the peripheral for the appropriate node role */
void Lin_TransitionToWakeUpState()
{
    /* Mark channel as fully operational after wakeup */
    gLIN.status = LIN_OPERATIONAL;
    /* For responder: reconfigure and await the next break field */
    if(gLIN.node == LIN_RESPONDER)
    {
        /* Reconfigure peripheral to responder receive mode */
        Lin_ConfigResponder();
        /* Return state machine to break-detection */
        gLIN.state = LIN_RX_STATE_WAIT_FOR_BREAK;
    }
    /* For commander: return to idle transmit state ready for the next LIN_SendData call */
    else if (gLIN.node == LIN_COMMANDER)
    {
        /* Return state machine to wait-for-send-frame */
        gLIN.state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
        /* Reconfigure peripheral to commander transmit mode */
        Lin_ConfigCommander();
    }
}

/* Lin_EnableWakeupDetection: prepares the peripheral to detect an incoming wakeup
 * pulse by enabling the positive-edge interrupt on the RX line */
void Lin_EnableWakeupDetection()
{
    /* Enter the wakeup-detection state so the ISR knows how to handle the rising edge */
    gLIN.state = LIN_STATE_WAIT_FOR_WAKEUP;
    /* Clear any previously detected external wakeup flag */
    gLIN.extWakeUpStatus = false;
    /* Clear any stale positive-edge flag before enabling the interrupt */
    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_INTERRUPT_RXD_POS_EDGE);
    /* Enable positive-edge interrupt to detect the end of the wakeup pulse */
    DL_UART_enableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_RXD_POS_EDGE);
}

/* Lin_Common_updateReg: performs a read-modify-write on a volatile 32-bit peripheral register,
 * clearing the bits selected by mask and setting them to the corresponding bits of val */
void Lin_Common_updateReg(volatile uint32_t *reg, uint32_t val, uint32_t mask)
{
    uint32_t tmp;

    /* Read current register value */
    tmp  = *reg;
    /* Clear only the bits covered by mask */
    tmp  = tmp & ~mask;
    /* Write back with the new value applied only within the masked bits */
    *reg = tmp | (val & mask);
}

/* Lin_ConfigResponder: initializes the LIN peripheral and global context for
 * responder (slave) node operation, enabling break, falling-edge, overflow,
 * and positive-edge interrupts and arming the counter for break detection */
void Lin_ConfigResponder()
{
    /* Set state machine to wait for the next break field */
    gLIN.state = LIN_RX_STATE_WAIT_FOR_BREAK;
    /* Record this node as a responder */
    gLIN.node = LIN_RESPONDER;
    /* Disable rising-edge interrupt; responder does not use it in normal receive mode */
    DL_UART_disableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_LIN_RISING_EDGE);
    /* Clear all relevant interrupt flags accumulated from the previous frame or error */
    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_INTERRUPT_BREAK_ERROR |
                                             DL_UART_INTERRUPT_LIN_FALLING_EDGE |
                                             DL_UART_INTERRUPT_LIN_RISING_EDGE |
                                             DL_UART_INTERRUPT_LIN_COUNTER_OVERFLOW |
                                             DL_UART_INTERRUPT_RXD_POS_EDGE);

    /* Disable the UART receiver to prevent spurious RX bytes during peripheral reconfiguration */
    Lin_Common_updateReg(&LIN_0_INST->CTL0, UART_CTL0_RXE_DISABLE, UART_CTL0_RXE_MASK);

    /* Flush any bytes left in the RX FIFO from the previous frame */
    while(!DL_UART_isRXFIFOEmpty(LIN_0_INST))
    {
        DL_UART_receiveData(LIN_0_INST);
    }

    /* Disable compare match while reconfiguring for break detection mode */
    DL_UART_disableLINCounterCompareMatch(LIN_0_INST);
    /* Count only while bus is low so the counter measures break field duration */
    DL_UART_enableLINCountWhileLow(LIN_0_INST);
    /* Clear counter on each falling edge so each low pulse is measured independently */
    DL_UART_enableLINCounterClearOnFallingEdge(LIN_0_INST);
    /* Zero the counter before re-enabling */
    DL_UART_setLINCounterValue(LIN_0_INST, 0);
    /* Start the counter to begin monitoring for a break field */
    DL_UART_enableLINCounter(LIN_0_INST);

    /* Enable the interrupts needed for break detection and frame reception */
    DL_UART_enableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_BREAK_ERROR |
                                        DL_UART_INTERRUPT_LIN_FALLING_EDGE |
                                        DL_UART_INTERRUPT_LIN_COUNTER_OVERFLOW |
                                        DL_UART_INTERRUPT_RXD_POS_EDGE);

    /* If this configuration follows an error, invalidate the previous SYNC measurement
     * so the next frame performs a full baud rate re-validation */
    if(gLIN.errorResetScenario == true)
    {
        gLINT.firstSyncDone = false;
    }
    /* Clear the error-reset flag now that reconfiguration is complete */
    gLIN.errorResetScenario = false;
}

/* Lin_ConfigCommander: initializes the LIN peripheral and global context for
 * commander (master) node operation, disabling break-detection counter mode
 * and preparing the compare value for inter-byte timeout monitoring */
void Lin_ConfigCommander()
{
    /* Set state machine to idle, waiting for the application to call LIN_SendData */
    gLIN.state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
    /* Record this node as a commander */
    gLIN.node = LIN_COMMANDER;
    /* Clear any pending falling-edge and RX interrupt flags from the previous operation */
    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_INTERRUPT_LIN_FALLING_EDGE | DL_UART_INTERRUPT_RX);
    /* Stop the LIN counter; commander does not need it between frames */
    DL_UART_disableLINCounter(LIN_0_INST);
    /* Zero the counter for a clean state */
    DL_UART_setLINCounterValue(LIN_0_INST, 0);
    /* Disable count-while-low; commander counter mode is free-running for timeout */
    DL_UART_disableLINCountWhileLow(LIN_0_INST);
    /* Preload the compare value for use as an inter-byte timeout during the next frame */
    DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_TIMEOUT_DURATION);
    /* Enable compare match interrupt so the timeout can be detected */
    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
    /* Clear flags again after compare configuration to remove any glitch triggers */
    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_INTERRUPT_LIN_FALLING_EDGE | DL_UART_INTERRUPT_RX);
    /* Disable falling-edge and RX interrupts; commander does not use them in idle state */
    DL_UART_disableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_LIN_FALLING_EDGE | DL_UART_INTERRUPT_RX);
}

/* LIN_HeaderIndication: searches the message table for an entry matching the PID in rxPdu,
 * populates the checksum model, direction, data length, and callback fields on a match,
 * and returns true if found or false if the PID is not in the table */
uint8_t LIN_HeaderIndication(Lin_PduType *rxPdu)
{
    /* Iterate over all entries in the responder message table */
    for(int i = 0; i < LIN_RESPONDER_NUM_MSGS; i++)
    {
        /* Compare received PID against each table entry's pre-computed PID */
        if(rxPdu->Pid == gLIN.msgTbl[i].msgPID)
        {
            /* Copy checksum model from the matched table entry */
            rxPdu->Cs = gLIN.msgTbl[i].checksumType;
            /* Copy frame response direction from the matched table entry */
            rxPdu->Drc = gLIN.msgTbl[i].direction;
            /* Copy data length from the matched table entry */
            rxPdu->Dl = gLIN.msgTbl[i].msgSize;
            /* Store the associated callback function pointer in the global context */
            gLIN.callbackFunction = gLIN.msgTbl[i].callbackFunction;
            /* Return success: PID found in table */
            return true;
        }
    }
    /* Return failure: PID not found in table */
    return false;
}

/* Lin_receiveMessage: processes one received byte through the RX state machine,
 * advancing through SYNC -> ID -> DATA -> CHECKSUM states and triggering
 * callbacks or error handling as appropriate */
void Lin_receiveMessage(LIN_Peripheral_t peripheral, uint8_t rxByte, Lin_TxRxCtx_t *lin)
{
    uint8_t checksum = 0;
    uint8_t rxChecksum = 0;

    switch(gLIN.state)
    {
        /* SYNC byte received: transition to ID state and configure inter-byte timeout */
        case LIN_RX_STATE_SYNC:
            /* Advance state machine past SYNC byte to await the PID */
            gLIN.state = LIN_RX_STATE_ID;
            /* Reset counter for the inter-byte timeout measurement */
            DL_UART_setLINCounterValue(LIN_0_INST, 0);
            /* Stop count-while-low; timeout counter runs freely */
            DL_UART_disableLINCountWhileLow(LIN_0_INST);
            /* Load inter-byte timeout compare value */
            DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_TIMEOUT_DURATION);
            /* Mark that inter-byte timeout is now active */
            lin->timeoutEnabledForInterByte = true;
            /* Enable compare-match interrupt to fire on timeout */
            DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
            /* Clear falling-edge flag before re-enabling to avoid stale triggers */
            DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
            /* Disable counter-clear-on-falling-edge for free-running timeout mode */
            DL_UART_disableLINCounterClearOnFallingEdge(LIN_0_INST);
            /* Restart counter to start the inter-byte timeout window */
            DL_UART_enableLINCounter(LIN_0_INST);
            /* Re-enable falling-edge interrupt to detect the next byte start bit */
            DL_UART_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
            break;
        
        /* PID byte received: validate parity, look up frame in table, decide TX or RX */
        case LIN_RX_STATE_ID:
            /* Tentatively advance to DATA state (may be overridden to TX_STATE_DATA) */
            lin->state = LIN_RX_STATE_DATA;
            /* Responder-specific PID processing */
            if(lin->node == LIN_RESPONDER)
            {
                /* Validate the two parity bits embedded in the PID */
                if(LIN_validatePID(rxByte) == false)
                {
                    /* Flag error reset and transition to reset state on next falling edge */
                    gLIN.errorResetScenario = true;
                    gLIN.state = LIN_STATE_RESET_TRANSITION;
                    LIN_ResetState(LIN_0_INST, &gLIN);
                    break;
                }
                /* Store the received PID for checksum calculation */
                lin->rxPdu.Pid = rxByte;
                /* Point SduPtr to the shadow buffer so TX data can be sourced from it */
                lin->rxPdu.SduPtr = lin->txPdu.shadowBuffer;

                /* Look up the PID in the message table to determine direction and config */
                uint8_t LinIfStatus = LIN_HeaderIndication(&lin->rxPdu);

                /* Reset running checksum accumulator for this frame */
                lin->tempChksum.word = 0;
                /* For enhanced checksum, seed the accumulator with the PID */
                if(lin->rxPdu.Cs == LIN_ENHANCED_CS)
                {
                    lin->tempChksum.word = lin->rxPdu.Pid;
                }

                /* If this frame requires the responder to transmit the response */
                if(LinIfStatus == true && lin->rxPdu.Drc == LIN_FRAMERESPONSE_TX)
                {
                    /* Switch to TX data state to begin transmitting response bytes */
                    lin->state = LIN_TX_STATE_DATA;
                    /* Mark channel as busy transmitting */
                    lin->status = LIN_TX_BUSY;
                    /* Copy data length from RX PDU to TX PDU */
                    lin->txPdu.Dl = lin->rxPdu.Dl;
                    /* Reset TX byte counter for this frame */
                    lin->txPdu.byteCounter = 0;

                    /* Reconfigure counter for inter-byte timeout during TX */
                    DL_UART_setLINCounterValue(LIN_0_INST, 0);
                    DL_UART_disableLINCountWhileLow(LIN_0_INST);
                    DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_TIMEOUT_DURATION);
                    lin->timeoutEnabledForInterByte = true;
                    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
                    /* Clear both falling-edge and RX flags before enabling interrupts */
                    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE | DL_UART_INTERRUPT_RX);
                    DL_UART_disableLINCounterClearOnFallingEdge(LIN_0_INST);
                    DL_UART_enableLINCounter(LIN_0_INST);
                    /* Enable both falling-edge and RX interrupts for loopback verification */
                    DL_UART_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE | DL_UART_INTERRUPT_RX);

                    /* Cache the first byte to be transmitted for loopback comparison */
                    lin->Tx_LAST_BYTE = lin->txPdu.shadowBuffer[lin->txPdu.byteCounter];

                    /* Insert half-bit delay between PID stop bit and first data byte start bit */
                    delay_cycles((gLINT.btWidth * (CPUCLK_FREQ / LIN_0_INST_FREQUENCY)) >> 1);

                    /* Disable and clear RX interrupt before enabling NEG edge for loopback */
                    DL_UART_disableInterrupt(LIN_0_INST, DL_UART_INTERRUPT_RX);
                    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_INTERRUPT_RX);
                    /* Arm negative-edge interrupt to detect the loopback start bit */
                    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                    DL_UART_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);

                    /* Transmit the first data byte */
                    DL_UART_transmitData(peripheral, lin->txPdu.shadowBuffer[lin->txPdu.byteCounter]);
                    /* Accumulate the transmitted byte into the running checksum */
                    lin->tempChksum.word += lin->txPdu.shadowBuffer[lin->txPdu.byteCounter];
                    /* Advance the TX byte pointer */
                    lin->txPdu.byteCounter++;
                    break;
                }
                /* If PID was not found or frame is marked IGNORE, skip the response */
                else if(LinIfStatus == false || lin->rxPdu.Drc == LIN_FRAMERESPONSE_IGNORE)
                {
                    /* Force direction to IGNORE so data bytes are not processed */
                    lin->rxPdu.Drc = LIN_FRAMERESPONSE_IGNORE;
                }
                else
                {
                /* Direction is RX: fall through to configure RX byte reception below */
                }
            }

            /* Prepare to receive data bytes (both RX frames and IGNORE frames) */
            /* Reset RX byte counter for the incoming data field */
            lin->rxPdu.byteCounter = 0;
            /* Reset counter for the inter-byte timeout of the first data byte */
            DL_UART_setLINCounterValue(LIN_0_INST, 0);
            DL_UART_disableLINCountWhileLow(LIN_0_INST);
            DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_TIMEOUT_DURATION);
            lin->timeoutEnabledForInterByte = true;
            DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
            /* Clear falling-edge flag before re-enabling */
            DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
            /* Disable counter-clear-on-falling-edge only for responder (free-running timeout) */
            if(lin->node == LIN_RESPONDER)
                DL_UART_disableLINCounterClearOnFallingEdge(LIN_0_INST);
            /* Restart counter for inter-byte timeout monitoring */
            DL_UART_enableLINCounter(LIN_0_INST);
            /* Enable falling-edge interrupt to detect start bit of each incoming data byte */
            DL_UART_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
            break;
        
        /* Data byte received: store in shadow buffer, accumulate checksum, advance counter */
        case LIN_RX_STATE_DATA:
            /* Mark channel as actively receiving data */
            lin->status = LIN_RX_BUSY;
            /* Store received byte in the shadow buffer at the current position */
            lin->rxPdu.shadowBuffer[lin->rxPdu.byteCounter] = rxByte;
            /* Accumulate byte into running checksum */
            lin->tempChksum.word += rxByte;
            /* Advance byte counter */
            lin->rxPdu.byteCounter++;
            /* If all expected data bytes have been received, transition to checksum state */
            if(lin->rxPdu.byteCounter >= lin->rxPdu.Dl)
            {
                lin->state = LIN_RX_STATE_CHECKSUM;
            }
            /* Reload inter-byte timeout for the next expected byte */
            DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_TIMEOUT_DURATION);
            lin->timeoutEnabledForInterByte = true;
            DL_UART_setLINCounterValue(LIN_0_INST, 0);
            DL_UART_enableLINCounter(LIN_0_INST);
            break;

        /* Checksum byte received: verify against computed checksum and invoke callbacks */
        case LIN_RX_STATE_CHECKSUM:
            /* Save the received checksum byte for comparison */
            rxChecksum = rxByte;
            /* Stop and clear the inter-byte timeout counter; frame reception is complete */
            DL_UART_setLINCounterValue(LIN_0_INST, 0);
            DL_UART_disableLINCounter(LIN_0_INST);

            /* Fold the 16-bit accumulator carry into the low byte */
            lin->tempChksum.word = lin->tempChksum.byte[0] + lin->tempChksum.byte[1];
            /* Fold any remaining carry from the fold step */
            checksum = lin->tempChksum.byte[0];
            checksum += lin->tempChksum.byte[1];
            /* Invert to produce the LIN checksum byte */
            checksum = 0xFF - checksum;

            /* Compare computed checksum against the received byte */
            if(rxChecksum == checksum)
            {
                /* Checksum matches: reception successful */
                lin->status = LIN_RX_OK;
            }
            else
            {
                /* Checksum mismatch: reception failed */
                lin->status = LIN_RX_ERROR;
            }

            /* Reset byte counter and checksum accumulator for the next frame */
            lin->rxPdu.byteCounter = 0;
            lin->tempChksum.word = 0;
            /* Mark reception as complete */
            lin->state = LIN_RX_STATE_COMPLETE;

            /* Responder post-reception handling */
            if(lin->node == LIN_RESPONDER)
            {
                /* Process only if this frame was not marked to be ignored */
                if(lin->rxPdu.Drc != LIN_FRAMERESPONSE_IGNORE)
                {
                    /* Successful RX: invoke the callback and return to idle */
                    if(lin->rxPdu.Drc == LIN_FRAMERESPONSE_RX && lin->status != LIN_RX_ERROR)
                    {
                        /* Invoke the application callback for this frame */
                        if(lin->callbackFunction != NULL)
                            lin->callbackFunction();
                        /* Return state machine to break-detection state */
                        lin->state = LIN_RX_STATE_WAIT_FOR_BREAK;
                        /* Reconfigure peripheral for the next frame */
                        Lin_ConfigResponder();
                    }
                    else
                    {
                        /* Checksum error or unexpected direction: report and reset */
                        LIN_SetError(lin, LIN_ERROR_CHECKSUM);
                        lin->errorResetScenario = true;
                        lin->state = LIN_STATE_RESET_TRANSITION;
                        LIN_ResetState(LIN_0_INST, &gLIN);
                    }
                }
                else
                {
                    /* Frame was IGNORE: silently return to break-detection state */
                    lin->state = LIN_RX_STATE_WAIT_FOR_BREAK;
                    Lin_ConfigResponder();
                }
            }
            /* Commander post-reception handling */
            if(lin->node == LIN_COMMANDER)
            {
                /* Checksum error: reset state machine */
                if(lin->status == LIN_RX_ERROR)
                {
                    lin->state = LIN_STATE_RESET_TRANSITION;
                    LIN_ResetState(LIN_0_INST, &gLIN);
                }
                else
                {
                    /* Successful RX: invoke callback and return to idle */
                    if(lin->callbackFunction != NULL)
                        lin->callbackFunction();
                    lin->state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
                    Lin_ConfigCommander();
                }
            }
            break;
        /* Unexpected state: default to DATA reception to avoid locking the state machine */
        default:
            lin->state = LIN_RX_STATE_DATA;
            break;
    }
}

/* LIN_TransmitMessage: processes one TX interrupt event through the TX state machine,
 * transmitting SYNC -> PID -> DATA -> CHECKSUM bytes and handing off to RX mode
 * or calling callbacks as each phase completes */
void LIN_TransmitMessage(LIN_Peripheral_t peripheral, Lin_TxRxCtx_t *lin)
{
    switch (lin->state) 
    {
        /* SYNC byte echo confirmed: transmit the PID byte next */
        case LIN_TX_STATE_SYNC:
            /* Advance to PID transmission state */
            lin->state = LIN_TX_STATE_PID;
            /* Reset checksum accumulator for this frame */
            lin->tempChksum.word = 0;
            /* For enhanced checksum, seed the accumulator with the PID */
            if(lin->txPdu.Cs == LIN_ENHANCED_CS)
            {
                lin->tempChksum.word = lin->txPdu.Pid;
            }
            
            /* Cache PID as the last-transmitted byte for loopback error detection */
            lin->Tx_LAST_BYTE = lin->txPdu.Pid;
            /* Reload inter-byte timeout compare value */
            DL_UART_setLINCounterCompareValue(peripheral, LIN_TIMEOUT_DURATION);
            lin->timeoutEnabledForInterByte = true;
            DL_UART_setLINCounterValue(peripheral, 0);
            DL_UART_enableLINCounter(peripheral);
            /* Transmit the Protected Identifier byte */
            DL_UART_transmitData(peripheral, lin->txPdu.Pid);
            break;

        /* PID echo confirmed: begin response phase based on frame direction */
        case LIN_TX_STATE_PID:
            /* Commander is transmitting the response: send first data byte */
            if(lin->txPdu.Drc == LIN_FRAMERESPONSE_TX)
            {
                lin->state = LIN_TX_STATE_DATA;
                /* Cache the first data byte for loopback verification */
                lin->Tx_LAST_BYTE = lin->txPdu.shadowBuffer[lin->txPdu.byteCounter];
                /* Reload inter-byte timeout */
                DL_UART_setLINCounterCompareValue(peripheral, LIN_TIMEOUT_DURATION);
                lin->timeoutEnabledForInterByte = true;
                DL_UART_setLINCounterValue(peripheral, 0);
                DL_UART_enableLINCounter(peripheral);
                /* Transmit the first data byte */
                DL_UART_transmitData(peripheral, lin->txPdu.shadowBuffer[lin->txPdu.byteCounter]);
                /* Accumulate via SduPtr (application buffer) for checksum */
                lin->tempChksum.word += lin->txPdu.SduPtr[lin->txPdu.byteCounter];
                lin->txPdu.byteCounter++;
            }
            /* Commander expects a response from the responder: switch to RX mode */
            if(lin->txPdu.Drc == LIN_FRAMERESPONSE_RX)
            {
                /* Copy PID, length, and direction into the RX PDU for incoming data processing */
                lin->rxPdu.Pid = lin->txPdu.Pid;
                lin->rxPdu.Dl = lin->txPdu.Dl;
                lin->rxPdu.Drc = lin->txPdu.Drc;
                /* Transition to RX data state to receive responder bytes */
                lin->state = LIN_RX_STATE_DATA;
                lin->rxPdu.byteCounter = 0;

                /* Flush any stale bytes from the RX FIFO before listening for the response */
                while(!DL_UART_isRXFIFOEmpty(peripheral))
                {
                    DL_UART_receiveData(peripheral);
                }

                /* Clear RX interrupt flag and arm inter-byte timeout for response reception */
                DL_UART_clearInterruptStatus(peripheral, DL_UART_INTERRUPT_RX);
                DL_UART_setLINCounterCompareValue(peripheral, LIN_TIMEOUT_DURATION);
                lin->timeoutEnabledForInterByte = true;
                DL_UART_setLINCounterValue(peripheral, 0);
                DL_UART_enableLINCounter(peripheral);
            }
            /* Frame direction is IGNORE: no response; return to idle immediately */
            if(lin->txPdu.Drc == LIN_FRAMERESPONSE_IGNORE)
            {
                lin->state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
                Lin_ConfigCommander();
                /* Report transmission as successful even though no data was sent */
                lin->status = LIN_TX_OK;
            }
            break;
            
        /* Data byte echo confirmed: transmit next data byte or switch to checksum */
        case LIN_TX_STATE_DATA:
            /* All data bytes transmitted: compute and send the checksum */
            if(lin->txPdu.byteCounter >= lin->txPdu.Dl)
            {
                lin->state = LIN_TX_STATE_CHECKSUM;
                /* Fold carry in the 16-bit accumulator */
                lin->tempChksum.word = lin->tempChksum.byte[0] + lin->tempChksum.byte[1];
                uint8_t checksum;
                /* Add any remaining carry from the fold */
                checksum = lin->tempChksum.byte[0];
                checksum += lin->tempChksum.byte[1];
                /* Invert to produce the LIN checksum */
                checksum = 0xFF - checksum;
                /* Cache checksum as the last byte for loopback verification */
                lin->Tx_LAST_BYTE = checksum;
                /* Reload inter-byte timeout for checksum byte */
                DL_UART_setLINCounterCompareValue(peripheral, LIN_TIMEOUT_DURATION);
                lin->timeoutEnabledForInterByte = true;
                DL_UART_setLINCounterValue(peripheral, 0);
                DL_UART_enableLINCounter(peripheral);
                /* Transmit the checksum byte */
                DL_UART_transmitData(peripheral, checksum);
                break;
            }

            /* More data bytes remain: transmit the next one */
            lin->Tx_LAST_BYTE = lin->txPdu.shadowBuffer[lin->txPdu.byteCounter];
            /* Reload inter-byte timeout for the next data byte */
            DL_UART_setLINCounterCompareValue(peripheral, LIN_TIMEOUT_DURATION);
            lin->timeoutEnabledForInterByte = true;
            DL_UART_setLINCounterValue(peripheral, 0);
            DL_UART_enableLINCounter(peripheral);
            /* Transmit the next data byte from the shadow buffer */
            DL_UART_transmitData(peripheral, lin->txPdu.shadowBuffer[lin->txPdu.byteCounter]);
            /* Accumulate the transmitted byte for checksum calculation */
            lin->tempChksum.word += lin->txPdu.shadowBuffer[lin->txPdu.byteCounter];
            lin->txPdu.byteCounter++;
            break;

        /* Checksum echo confirmed: frame transmission complete */
        case LIN_TX_STATE_CHECKSUM:
            /* Stop and reset the counter; no more bytes expected for this frame */
            DL_UART_disableLINCounter(peripheral);
            DL_UART_setLINCounterValue(peripheral, 0);

            /* Mark frame transmission as complete */
            lin->state = LIN_TX_STATE_COMPLETE;
            lin->status = LIN_TX_OK;
            /* Reset TX byte counter and checksum accumulator for the next frame */
            lin->txPdu.byteCounter = 0;
            lin->tempChksum.word = 0;

            /* For responder: notify application and return to receive mode */
            if(lin->node == LIN_RESPONDER)
            {
                /* Invoke the application TX complete callback */
                LIN_processMessage_Tx();
                /* Reconfigure peripheral for the next incoming frame */
                Lin_ConfigResponder();
                /* Return state machine to break-detection */
                gLIN.state = LIN_RX_STATE_WAIT_FOR_BREAK;
            }
            /* For commander: return to idle transmit state */
            else if(lin->node == LIN_COMMANDER)
            {
                gLIN.state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
                Lin_ConfigCommander();
            }
            break;
        
        /* No other TX states require action */
        default:
            break;
    }
}

/* LIN_SendData: accepts a PDU from the application, copies it into the global TX context,
 * generates the break and SYNC bytes, and arms the interrupt-driven state machine to
 * complete the rest of the frame; returns true on success, false on invalid parameters */
uint8_t LIN_SendData(Lin_PduType *txPdu)
{
    uint8_t return_value = false;
    /* If the state machine is not idle, force reset to commander idle state */
    if(gLIN.state != LIN_TX_STATE_WAIT_FOR_SEND_FRAME)
    {
        gLIN.state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
        Lin_ConfigCommander();
    }
    /* Block until the TX FIFO has room for at least one byte */
    while(DL_UART_isTXFIFOFull(LIN_0_INST) == true)
    {

    };

    /* Copy PDU metadata from the caller into the global TX PDU */
    gLIN.txPdu.Pid = txPdu->Pid;
    gLIN.txPdu.Cs = txPdu->Cs;
    gLIN.txPdu.Dl = txPdu->Dl;
    gLIN.txPdu.Drc = txPdu->Drc;

    /* For TX frames, validate and copy the data payload into the internal shadow buffer */
    if(gLIN.txPdu.Drc == LIN_FRAMERESPONSE_TX)
    {
        /* Validate that the caller provided a non-null buffer with a valid length */
        if(txPdu->SduPtr != NULL && txPdu->Dl > 0 && txPdu->Dl <= LIN_MAX_DATA_LENGTH)
        {
            /* Copy data into shadow buffer to protect against caller buffer modification during TX */
            memcpy(gLIN.txPdu.shadowBuffer, txPdu->SduPtr, txPdu->Dl);
            /* Point the internal SduPtr to the shadow buffer */
            gLIN.txPdu.SduPtr = gLIN.txPdu.shadowBuffer;
        }
        else
        {
            /* Invalid parameters: clear SduPtr and return failure */
            gLIN.txPdu.SduPtr = NULL;
            return_value = false;
            return return_value;
        }
    }

    /* Reset byte counter and checksum accumulator before starting the frame */
    gLIN.txPdu.byteCounter = 0;
    gLIN.tempChksum.word = 0;

    /* Mark channel as busy transmitting */
    gLIN.status = LIN_TX_BUSY;
    gLIN.channelStatus = LIN_CHANNEL_BUSY;
    /* Transition to break transmit state */
    gLIN.state = LIN_TX_STATE_BREAK;

    /* Assert break field: pull bus dominant for LIN_BREAK_BIT_COUNT bit times */
    DL_UART_enableLINSendBreak(LIN_0_INST);
    /* Hold break for the required duration in CPU clock cycles */
    delay_cycles(LIN_BREAK_BIT_COUNT * gLINT.btWidth * (CPUCLK_FREQ / LIN_0_INST_FREQUENCY));
    /* Release break: return bus to recessive */
    DL_UART_disableLINSendBreak(LIN_0_INST);

    /* Break complete: advance state machine to SYNC transmission */
    gLIN.state = LIN_TX_STATE_SYNC;

    /* Flush RX FIFO to remove any loopback bytes captured during the break field */
    while(!DL_UART_isRXFIFOEmpty(LIN_0_INST))
    {
        DL_UART_receiveData(LIN_0_INST);
    }

    /* Clear and enable falling-edge and negative-edge interrupts for SYNC loopback monitoring */
    DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE | DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
    DL_UART_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE | DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
    /* Set and enable the inter-byte timeout for the SYNC byte */
    DL_UART_setLINCounterCompareValue(LIN_0_INST, LIN_TIMEOUT_DURATION);
    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
    DL_UART_setLINCounterValue(LIN_0_INST, 0);
    DL_UART_enableLINCounter(LIN_0_INST);
    /* Cache SYNC byte as the last-transmitted byte for loopback comparison */
    gLIN.Tx_LAST_BYTE = LIN_SYNC_BYTE;
    /* Transmit the SYNC byte to allow responders to measure baud rate */
    DL_UART_transmitData(LIN_0_INST, LIN_SYNC_BYTE);

    /* Frame initiation successful */
    return_value = true;
    
    return return_value;
}

/* LIN_0_INST_IRQHandler: top-level UART LIN interrupt service routine;
 * dispatches to per-event handlers based on the highest-priority pending interrupt index */
void LIN_0_INST_IRQHandler(void)
{
    volatile uint8_t rxByte = 0;
    uint32_t counterVal = 0;

    switch (DL_UART_Extend_getPendingInterrupt(LIN_0_INST))
    {
    /* Falling edge detected on the LIN RX line */
    case DL_UART_EXTEND_IIDX_LIN_FALLING_EDGE:
        /* If the state machine is in the reset-transition state, complete the reset now */
        if (gLIN.state == LIN_STATE_RESET_TRANSITION)
        {
            /* For responder: complete reset to break-detection idle state */
            if(gLIN.node == LIN_RESPONDER)
            {
                gLIN.state = LIN_RX_STATE_WAIT_FOR_BREAK;
                Lin_ConfigResponder();
            }
            /* For commander: complete reset to transmit idle state */
            if(gLIN.node == LIN_COMMANDER)
            {
                gLIN.state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
                Lin_ConfigCommander();
            }
        }
        else
        {
            /* Normal operation: inter-byte timeout is NOT currently active */
            if(gLIN.timeoutEnabledForInterByte == false)
            {
                /* RX data phase: a falling edge signals the start bit of the next byte */
                if(gLIN.state == LIN_RX_STATE_SYNC || gLIN.state == LIN_RX_STATE_ID || gLIN.state == LIN_RX_STATE_DATA || gLIN.state == LIN_RX_STATE_CHECKSUM)
                {
                    /* Check whether the bus is still low (framing error suspect) */
                    if((DL_GPIO_readPins(GPIO_LIN_0_RX_PORT, GPIO_LIN_0_RX_PIN) >> 11) != STD_HIGH)
                    {
                        /* Bus is still low after the falling edge: possible stop-bit error */
                        gLIN.checkStopBitErrorOccured = true;
                        /* A stop bit had been detected previously, so this confirms a framing error */
                        if(gLIN.checkPosEdgeDetected == true)
                        {
                            /* Report stop-bit error only for data and checksum states */
                            if(gLIN.state != LIN_RX_STATE_SYNC && gLIN.state != LIN_RX_STATE_ID)
                            {
                                LIN_SetError(&gLIN, LIN_ERROR_RESP_STOPBIT);
                            }
                            /* Trigger error reset on the next falling edge */
                            gLIN.errorResetScenario = true;
                            gLIN.state = LIN_STATE_RESET_TRANSITION;
                            LIN_ResetState(LIN_0_INST, &gLIN);
                        }
                    }
                    else
                    {
                        /* Bus returned high before the start bit fully resolved: valid stop bit present */
                        /* Re-arm the negative-edge interrupt for the next byte (except after checksum) */
                        if(gLIN.state != LIN_RX_STATE_CHECKSUM)
                        {
                            DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                            DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                        }
                        /* Wait for the RX interrupt flag indicating the full byte has been received */
                        while(!DL_UART_Extend_getRawInterruptStatus(LIN_0_INST, UART_CPU_INT_RIS_RXINT_MASK));
                        /* Clear the RX interrupt flag */
                        DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);
                        /* Read the received byte from the FIFO */
                        rxByte = DL_UART_receiveData(LIN_0_INST);
                        /* Validate that the SYNC byte matches the expected 0x55 value */
                        if(gLIN.state == LIN_RX_STATE_SYNC)
                        {
                            if(rxByte != LIN_SYNC_BYTE)
                            {
                                /* SYNC byte mismatch: set header error and reset */
                                LIN_SetError(&gLIN, LIN_ERROR_HEADER);
                                gLIN.errorResetScenario = true;
                                gLIN.state = LIN_STATE_RESET_TRANSITION;
                                break;
                            }
                        }
                        /* Pass the received byte to the RX state machine */
                        Lin_receiveMessage(LIN_0_INST, rxByte, &gLIN);
                    }
                }
                /* TX loopback phase: a falling edge signals the start bit of a loopback byte */
                else if(gLIN.state == LIN_TX_STATE_SYNC || gLIN.state == LIN_TX_STATE_PID || gLIN.state == LIN_TX_STATE_DATA || gLIN.state == LIN_TX_STATE_CHECKSUM)
                {
                    /* Check that the bus returned high (valid stop bit on the previous byte) */
                    if((DL_GPIO_readPins(GPIO_LIN_0_RX_PORT, GPIO_LIN_0_RX_PIN) >> 11) == STD_HIGH)
                    {
                        /* Re-arm negative-edge interrupt for the next loopback byte (except after checksum) */
                        if(gLIN.state != LIN_TX_STATE_CHECKSUM)
                        {
                            DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                            DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                        }
                        /* Wait for the full loopback byte to be received in the FIFO */
                        while(DL_UART_Extend_getRawInterruptStatus(LIN_0_INST, UART_CPU_INT_RIS_RXINT_MASK) == false)
                        {
                            
                        }
                        /* Clear the RX interrupt flag after the byte is ready */
                        DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);
                        /* Read the loopback byte */
                        rxByte = DL_UART_receiveData(LIN_0_INST);
                        /* Compare the loopback byte against the last transmitted byte */
                        if(gLIN.Tx_LAST_BYTE != rxByte)
                        {
                            /* Loopback mismatch: stop counter and report bit error */
                            DL_UART_Extend_setLINCounterValue(LIN_0_INST,0);
                            DL_UART_Extend_disableLINCounter(LIN_0_INST);
                            /* For responder: set data-bit error and trigger reset */
                            if(gLIN.node == LIN_RESPONDER)
                            {
                                LIN_SetError(&gLIN, LIN_ERROR_RESP_DATABIT);
                                gLIN.errorResetScenario = true;
                                gLIN.state = LIN_STATE_RESET_TRANSITION;
                                LIN_ResetState(LIN_0_INST, &gLIN);
                            }
                            /* For commander: set TX error status and trigger reset */
                            if(gLIN.node == LIN_COMMANDER)
                            {
                                gLIN.status = LIN_TX_ERROR;
                                gLIN.state = LIN_STATE_RESET_TRANSITION;
                                LIN_ResetState(LIN_0_INST, &gLIN);
                            }
                            break;
                        }
                        /* Loopback matched: reset counter and insert half-bit delay before advancing */
                        DL_UART_Extend_setLINCounterValue(LIN_0_INST,0);
                        /* Responder uses initial bit width for the delay reference */
                        if(gLIN.node == LIN_RESPONDER)
                        {
                            delay_cycles((gLINT.intialBtWidth * (CPUCLK_FREQ / LIN_0_INST_FREQUENCY)) >> 1);
                        }
                        /* Commander uses current (possibly auto-baud updated) bit width */
                        if(gLIN.node == LIN_COMMANDER)
                        {
                            delay_cycles((gLINT.btWidth * (CPUCLK_FREQ / LIN_0_INST_FREQUENCY)) >> 1);
                        }
                        /* Advance the TX state machine to send the next byte */
                        LIN_TransmitMessage(LIN_0_INST, &gLIN);
                    }
                    else
                    {
                        /* Bus still low when stop bit was expected: framing error on TX loopback */
                        gLIN.checkStopBitErrorOccured = true;
                        /* A previous stop bit was detected, so this confirms a framing error */
                        if(gLIN.checkPosEdgeDetected == true)
                        {
                            LIN_SetError(&gLIN, LIN_ERROR_RESP_DATABIT);
                            gLIN.errorResetScenario = true;
                            gLIN.state = LIN_STATE_RESET_TRANSITION;
                            LIN_ResetState(LIN_0_INST, &gLIN);
                        }
                    }
                }
                else 
                {
                /* Falling edge in an unrelated state: no action required */
                }
            }
            /* Inter-byte timeout IS currently active: treat falling edge as a timeout event */
            else
            {
                /* Responder timeout handling */
                if(gLIN.node == LIN_RESPONDER)
                {
                    /* In early receive states, flag error reset for next frame */
                    if(gLIN.state == LIN_RX_STATE_SYNC_FIELD_NEG_EDGE || gLIN.state == LIN_RX_STATE_ID || gLIN.state == LIN_RX_STATE_SYNC)
                    {
                        gLIN.errorResetScenario = true;
                    }
                    /* For frames that expected a response, determine no-response vs incomplete */
                    if(gLIN.rxPdu.Drc != LIN_FRAMERESPONSE_IGNORE)
                    {
                        if(gLIN.state == LIN_RX_STATE_DATA || gLIN.state == LIN_RX_STATE_CHECKSUM)
                        {
                            /* Zero bytes received: no response at all */
                            if(gLIN.rxPdu.byteCounter == 0)
                            {
                                LIN_SetError(&gLIN, LIN_ERROR_NO_RESPONSE);
                                gLIN.errorResetScenario = true;
                            }
                            else
                            {
                                /* Some bytes received but not all: incomplete response */
                                LIN_SetError(&gLIN, LIN_ERROR_INCOMPLETE_RESPONSE);
                                gLIN.errorResetScenario = true;
                            }
                        }
                        else if(gLIN.state == LIN_TX_STATE_DATA || gLIN.state == LIN_TX_STATE_CHECKSUM)
                        {
                            /* Timeout during TX data phase indicates a bus contention or bit error */
                            LIN_SetError(&gLIN, LIN_ERROR_RESP_DATABIT);
                            gLIN.errorResetScenario = true;
                        }
                        else
                        {
                        /* No specific error action for other states */
                        }
                    }
                    /* Return responder to break-detection idle state */
                    gLIN.state = LIN_RX_STATE_WAIT_FOR_BREAK;
                    Lin_ConfigResponder();
                }
                /* Commander timeout handling */
                if(gLIN.node == LIN_COMMANDER)
                {
                    /* Header timeout: SYNC or PID was not echoed in time */
                    if(gLIN.state == LIN_TX_STATE_SYNC || gLIN.state == LIN_TX_STATE_PID)
                    {
                        gLIN.status = LIN_TX_HEADER_ERROR;
                    }
                    /* Data timeout: data or checksum byte was not echoed in time */
                    if(gLIN.state == LIN_TX_STATE_DATA || gLIN.state == LIN_TX_STATE_CHECKSUM)
                    {
                        gLIN.status = LIN_TX_ERROR;
                    }
                    /* RX timeout: response from responder did not arrive or was incomplete */
                    if(gLIN.state == LIN_RX_STATE_DATA || gLIN.state == LIN_RX_STATE_CHECKSUM)
                    {
                        /* No bytes received from responder */
                        if(gLIN.rxPdu.byteCounter == 0)
                        {
                            gLIN.status = LIN_RX_NO_RESPONSE;
                        }
                        else
                        {
                            /* Partial response: some bytes received but frame did not complete */
                            gLIN.status = LIN_RX_ERROR;
                        }
                    }
                    /* Return commander to transmit idle state */
                    gLIN.state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
                    DL_UART_Extend_disableLINCounter(LIN_0_INST);
                    DL_UART_Extend_setLINCounterValue(LIN_0_INST,0);
                    Lin_ConfigCommander();
                }
            }
        }
        break;

    /* Rising edge detected: used to verify stop bits and detect framing errors */
    case DL_UART_EXTEND_IIDX_LIN_RISING_EDGE:
        /* Process rising edge only during active data or checksum phases */
        if(gLIN.state == LIN_RX_STATE_SYNC || gLIN.state == LIN_RX_STATE_ID || gLIN.state == LIN_RX_STATE_DATA ||gLIN.state == LIN_RX_STATE_CHECKSUM ||gLIN.state == LIN_TX_STATE_DATA ||gLIN.state == LIN_TX_STATE_CHECKSUM)
        {
            /* A previous falling edge had indicated a possible stop-bit error */
            if(gLIN.checkStopBitErrorOccured == true)
            {
                /* Report stop-bit or data-bit error based on current state */
                if(gLIN.state != LIN_RX_STATE_ID && gLIN.state != LIN_RX_STATE_SYNC)
                {
                    /* RX states: report a stop-bit error */
                    if(gLIN.state == LIN_RX_STATE_DATA || gLIN.state == LIN_RX_STATE_CHECKSUM)
                    {
                        LIN_SetError(&gLIN, LIN_ERROR_RESP_STOPBIT);
                    }
                    /* TX states: report a data-bit error from the TX loopback */
                    if(gLIN.state == LIN_TX_STATE_DATA || gLIN.state == LIN_TX_STATE_CHECKSUM)
                    {
                        LIN_SetError(&gLIN, LIN_ERROR_RESP_DATABIT);
                    }
                }
                /* Measure the low-pulse width to determine if it could be a new break field */
                uint16_t negToPosEdgeCount = DL_UART_Extend_getLINCounterValue(LIN_0_INST);
                /* If the low pulse is within the maximum break duration, treat it as a new break */
                if(negToPosEdgeCount <= (LIN_TBIT_COUNTER_COEFFICIENT_MAX * gLINT.intialBtWidth))
                {
                    /* Transition to SYNC measurement state to re-detect baud rate */
                    gLIN.state = LIN_RX_STATE_SYNC_FIELD_NEG_EDGE;
                    Lin_enableFeaturesForSyncCheck();
                    /* Reset all SYNC measurement accumulators */
                    gLINT.btSum = 0;
                    gLINT.autoBaud = false;
                    gLINT.syncErrCnt = 0;
                    gLINT.numCyclesPos = 0;
                    gLINT.numCyclesNeg = 0;
                }
            }
            else
            {
                /* No prior stop-bit error: mark that a valid stop-bit rising edge was seen */
                gLIN.checkPosEdgeDetected = true;
            }

            /* Disable and clear rising-edge interrupt after handling to avoid re-entry */
            DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
            DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
        }
        break;

    /* Positive (rising) edge on RXD: used for break-end detection and wakeup detection */
    case DL_UART_EXTEND_IIDX_RXD_POS_EDGE:
        /* Wakeup detection: validate the wakeup pulse width while in sleep state */
        if(gLIN.status == LIN_CH_SLEEP && gLIN.state == LIN_STATE_WAIT_FOR_WAKEUP)
        {
            /* Read the counter value that measured the dominant pulse width */
            counterVal = DL_UART_Extend_getLINCounterValue(LIN_0_INST);
            /* Accept as valid wakeup if pulse width is within the 250 µs–5 ms range */
            if((counterVal >= (LIN_WAKE_UP_MIN_TIME_IN_US * (LIN_0_INST_FREQUENCY / LIN_SCALE_IN_MHZ))) && 
               (counterVal <= (LIN_WAKE_UP_MAX_TIME_IN_US * (LIN_0_INST_FREQUENCY / LIN_SCALE_IN_MHZ))))
            {
                /* Flag that an external wakeup was detected */
                gLIN.extWakeUpStatus = true;
                /* Notify application of wakeup event */
                LIN_processWakeUp();
            }
        }
        /* Break-end detection: rising edge ends the break field */
        else if(gLIN.state == LIN_RX_STATE_WAIT_FOR_BREAK)
        {
            /* Advance state machine past break detection */
            gLIN.state = LIN_RX_STATE_BREAK_FIELD;

            /* Read the measured break field duration in counter cycles */
            counterVal = DL_UART_Extend_getLINCounterValue(LIN_0_INST);
            /* Use current and limit bit widths to determine validation bounds */
            uint32_t tbit_width = gLINT.btWidth;
            uint32_t tbit_width_limit = gLINT.btWidthLimit;

            uint32_t min_tbit_width;
            uint32_t max_tbit_width;

            /* Determine which is the smaller and larger of the two bit widths */
            if(tbit_width <= tbit_width_limit)
            {
                min_tbit_width = tbit_width;
                max_tbit_width = tbit_width_limit;
            }
            else
            {
                min_tbit_width = tbit_width_limit;
                max_tbit_width = tbit_width;
            }

            /* Validate break duration against min/max bounds depending on auto-baud mode */
            if(((AUTO_BAUD_ENABLED == false) && (counterVal <= (tbit_width * LIN_TBIT_COUNTER_COEFFICIENT_MAX)) &&
            ((counterVal << 10U) >= (tbit_width * LIN_TBIT_COUNTER_COEFFICIENT_MIN * 1009U ))) ||
            ((AUTO_BAUD_ENABLED == true) && (counterVal <= (max_tbit_width * LIN_TBIT_COUNTER_COEFFICIENT_MAX)) &&
            ((counterVal << 10U) >= (min_tbit_width * LIN_TBIT_COUNTER_COEFFICIENT_MIN * 881U))))
            {
                /* Break is valid: decide whether SYNC measurement is needed */
                /* If SYNC-check-first-frame mode is selected and SYNC was already validated, skip re-measurement */
                if((gLIN.syncCheckType == LIN_SYNC_CHECK_FIRST_FRAME) && gLINT.firstSyncDone == true)
                {
                    /* Skip SYNC field measurement: go directly to SYNC byte reception */
                    DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_POS_EDGE);
                    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_POS_EDGE);
                    /* Arm negative-edge interrupt to detect SYNC byte start bit */
                    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                    /* Enable the UART receiver to accept the SYNC byte */
                    Lin_Common_updateReg(&LIN_0_INST->CTL0, UART_CTL0_RXE_ENABLE, UART_CTL0_RXE_MASK);

                    /* Configure counter for inter-byte timeout monitoring */
                    DL_UART_disableLINCountWhileLow(LIN_0_INST);
                    DL_UART_disableLINCounterClearOnFallingEdge(LIN_0_INST);
                    DL_UART_setLINCounterCompareValue(LIN_0_INST,LIN_TIMEOUT_DURATION);
                    gLIN.timeoutEnabledForInterByte = true;
                    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
                    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
                    DL_UART_setLINCounterValue(LIN_0_INST, 0);
                    DL_UART_enableLINCounter(LIN_0_INST);
                    /* Enable falling-edge interrupt to detect SYNC byte start bit */
                    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);

                    /* Advance to SYNC reception state */
                    gLIN.state = LIN_RX_STATE_SYNC;
                }
                else
                {
                    /* Full SYNC field measurement required: arm edge-capture interrupts */
                    DL_UART_enableLINCounterClearOnFallingEdge(LIN_0_INST);
                    DL_UART_disableLINCountWhileLow(LIN_0_INST);
                    /* Arm negative-edge interrupt for first SYNC bit edge capture */
                    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
                    /* Disable falling-edge interrupt; edge-capture interrupts replace it */
                    DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
                    /* Enable sync field validation counter control for hardware-assisted capture */
                    DL_UART_enableLINSyncFieldValidationCounterControl(LIN_0_INST);
                    /* Disable raw RX interrupt during SYNC measurement */
                    DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);

                    /* Set timeout compare value and enable it to detect a missing SYNC field */
                    DL_UART_setLINCounterCompareValue(LIN_0_INST,LIN_TIMEOUT_DURATION);
                    gLIN.timeoutEnabledForInterByte = true;
                    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
                    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
                    DL_UART_setLINCounterValue(LIN_0_INST, 0);
                    DL_UART_enableLINCounter(LIN_0_INST);
                    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
                    /* Enter SYNC measurement state to capture edge timings */
                    gLIN.state = LIN_RX_STATE_SYNC_FIELD_NEG_EDGE;
                }
                /* Reset all SYNC measurement accumulators for this new frame */
                gLINT.btSum = 0;
                gLINT.autoBaud = false;
                gLINT.syncErrCnt = 0;
                gLINT.numCyclesPos = 0;
                gLINT.numCyclesNeg = 0;
            }
            else
            {
                /* Break duration out of range: report header error and reset */
                LIN_SetError(&gLIN, LIN_ERROR_HEADER);
                gLIN.errorResetScenario = true;
                gLIN.state = LIN_STATE_RESET_TRANSITION;
                DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RX);
                LIN_ResetState(LIN_0_INST, &gLIN);
            }
        }

        /* Rising edge during SYNC field measurement */
        else
        {
            /* Process positive edges only in the SYNC positive-edge capture state */
            if(gLIN.state == LIN_RX_STATE_SYNC_FIELD_POS_EDGE)
            {
                /* Auto-baud recalibration was applied: SYNC measurement is complete */
                if(gLINT.autoBaud == true)
                {
                    /* Re-enable UART receiver with newly calibrated baud rate */
                    Lin_Common_updateReg(&LIN_0_INST->CTL0, UART_CTL0_RXE_ENABLE, UART_CTL0_RXE_MASK);
                    /* Advance to PID reception state */
                    gLIN.state = LIN_RX_STATE_ID;
                    /* Disable positive-edge interrupt; no more SYNC edges expected */
                    DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_POS_EDGE);

                    /* Configure counter for inter-byte timeout between PID and data */
                    DL_UART_disableLINCounterClearOnFallingEdge(LIN_0_INST);
                    DL_UART_setLINCounterCompareValue(LIN_0_INST,LIN_TIMEOUT_DURATION);
                    gLIN.timeoutEnabledForInterByte = true;
                    DL_UART_enableLINCounterCompareMatch(LIN_0_INST);
                    DL_UART_Extend_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
                    DL_UART_setLINCounterValue(LIN_0_INST, 0);
                    DL_UART_enableLINCounter(LIN_0_INST);
                    /* Re-enable falling-edge interrupt to detect PID byte start bit */
                    DL_UART_Extend_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
                }
                else
                {
                    /* No auto-baud: count this positive edge and return to NEG edge capture state */
                    gLINT.numCyclesPos++;
                    gLIN.state = LIN_RX_STATE_SYNC_FIELD_NEG_EDGE;
                    /* On the first positive edge, validate the half-SYNC interval width */
                    if(gLINT.numCyclesPos == 1U)
                    {
                        /* Read the time between the last neg and this pos edge */
                        uint16_t syncNegToPosCount = DL_UART_Extend_getLINRisingEdgeCaptureValue(LIN_0_INST);
                        /* Check if the half-bit interval is within the expected break-to-sync range */
                        if((syncNegToPosCount >= ((19U * gLINT.intialBtWidth) >> 1U)) && (syncNegToPosCount <= (LIN_TBIT_COUNTER_COEFFICIENT_MAX * gLINT.intialBtWidth)))
                        {
                            /* Interval exceeds expected SYNC range: restart SYNC measurement */
                            Lin_enableFeaturesForSyncCheck();
                            gLINT.btSum = 0;
                            gLINT.autoBaud = false;
                            gLINT.syncErrCnt = 0;
                            gLINT.numCyclesPos = 0;
                            gLINT.numCyclesNeg = 0;
                        }
                    }
                }
            }
        }
        break;

    /* LIN counter overflow: indicates the counter wrapped without detecting a valid event */
    case DL_UART_EXTEND_IIDX_LIN_COUNTER_OVERFLOW:
        /* Responder overflow: likely no break field arrived within the expected window */
        if(gLIN.node == LIN_RESPONDER)
        {
            /* Report as a header error and return to break-detection idle */
            LIN_SetError(&gLIN, LIN_ERROR_HEADER);
            gLIN.errorResetScenario = true;
            gLIN.state = LIN_RX_STATE_WAIT_FOR_BREAK;
            Lin_ConfigResponder();
        }
        /* Commander overflow: no echo received within the timeout window */
        if(gLIN.node == LIN_COMMANDER)
        {
            /* Return commander to idle transmit state */
            gLIN.state = LIN_TX_STATE_WAIT_FOR_SEND_FRAME;
            Lin_ConfigCommander();
        }
        break;

    /* Negative (falling) edge on RXD: used for SYNC field bit-time measurement */
    case DL_UART_EXTEND_IIDX_RXD_NEG_EDGE:
        /* Process negative edges only during SYNC field measurement */
        if(gLIN.state == LIN_RX_STATE_SYNC_FIELD_NEG_EDGE)
        {
            /* Disable compare match temporarily to allow edge-capture without false timeout */
            DL_UART_disableLINCounterCompareMatch(LIN_0_INST);
            /* Disable falling-edge interrupt during edge-capture processing */
            DL_UART_Extend_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
            DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_FALLING_EDGE);
            /* Re-enable the sync field validation counter control for hardware-assisted timing */
            DL_UART_enableLINSyncFieldValidationCounterControl(LIN_0_INST);

            /* Store the captured falling-edge timer value for this SYNC cycle */
            gLINT.syncBt[gLINT.numCyclesNeg].negEdge = DL_UART_Extend_getLINFallingEdgeCaptureValue(LIN_0_INST);
            /* Reference bit width for tolerance check */
            uint32_t tbit_width_neg = gLINT.intialBtWidth;
            /* Process measurement only if at least one positive edge has been seen */
            if(gLINT.numCyclesPos > 0)
            {
                uint16_t tempNumCyclesNeg = gLINT.numCyclesNeg;
                uint16_t tempNumCyclesPos = gLINT.numCyclesPos;
                /* Verify that neg and pos edge counts are properly interleaved */
                if(tempNumCyclesNeg == (tempNumCyclesPos - (uint16_t)1))
                {
                    uint16_t tempBitTimesNegEdge = gLINT.syncBt[gLINT.numCyclesNeg].negEdge;
                    /* Accumulate the measured neg-to-neg interval into the bit-time sum */
                    gLINT.btSum +=  tempBitTimesNegEdge;
                    /* Compute the half-period from neg-edge to previous pos-edge */
                    uint16_t avgPosNeg = (gLINT.syncBt[gLINT.numCyclesNeg].negEdge >> 1U);
                    /* Check if the half-period is within the 98.5%–101.5% tolerance band */
                    if (!(((avgPosNeg << 10U) > (tbit_width_neg * 1009U)) && ((avgPosNeg << 10U) < (tbit_width_neg * 1039U))))
                    {
                        /* Half-period out of tolerance: increment error counter */
                        gLINT.syncErrCnt++;
                    }
                    /* Advance to the next neg edge slot */
                    gLINT.numCyclesNeg++;
                }
                else
                {
                    /* Edge count mismatch: SYNC field is malformed; report header error and reset */
                    LIN_SetError(&gLIN, LIN_ERROR_HEADER);
                    gLIN.errorResetScenario = true;
                    gLIN.state = LIN_STATE_RESET_TRANSITION;
                    LIN_ResetState(LIN_0_INST, &gLIN);
                }
            }
            
            /* Check if all required SYNC cycles have been measured */
            if(gLINT.numCyclesNeg == LIN_RESPONDER_SYNC_CYCLES)
            {
                /* More errors than the threshold: baud rate has changed, recalibrate */
                if(gLINT.syncErrCnt >= AUTO_BAUD_THRESHOLD)
                {
                    if(AUTO_BAUD_ENABLED == true)
                    {
                        /* Compute rounded average bit time from accumulated sum of 4 cycles × 2 edges */
                        uint32_t roundedTbit = (gLINT.btSum >> 3);
                        /* Round up if the 4th bit of the sum is set */
                        if((gLINT.btSum & 4U) == 4U)
                        {
                            roundedTbit += (uint32_t)1U;
                        }
                        /* UART uses 16× oversampling: IBRD = roundedTbit / 16 */
                        uint32_t overSamplingRate = 16;
                        uint32_t newIbrd = roundedTbit / overSamplingRate;
                        /* Compute remainder for fractional baud rate divider */
                        uint32_t rem = roundedTbit % overSamplingRate;
                        /* FBRD uses 6-bit fractional field (64 steps) */
                        uint32_t frac_scale = 64;
                        /* FBRD = round(rem * 64 / 16) using integer arithmetic */
                        uint64_t num = ((uint64_t)rem * (uint64_t)frac_scale) + (uint64_t)((uint64_t)overSamplingRate / 2U);
                        uint32_t newFbrd = (uint32_t)(num / (uint64_t)overSamplingRate);
                        /* Clamp FBRD to maximum valid value */
                        if(newFbrd > (frac_scale - 1U))
                        {
                            newFbrd = frac_scale - 1U;
                        }
                        /* Write the new integer baud rate divisor to the UART register */
                        Lin_Common_updateReg(&LIN_0_INST->IBRD, (uint32_t) newIbrd, UART_IBRD_DIVINT_MASK);
                        /* Write the new fractional baud rate divisor to the UART register */
                        Lin_Common_updateReg(&LIN_0_INST->FBRD, (uint32_t) newFbrd, UART_FBRD_DIVFRAC_MASK);
                        /* Trigger baud rate latch by preserving BRK bit and writing LCRH */
                        uint32_t value = (LIN_0_INST->LCRH & UART_LCRH_BRK_MASK);
                        Lin_Common_updateReg(&LIN_0_INST->LCRH, value, UART_LCRH_BRK_MASK);
                        /* update the tbit value for next iterations/frames */
                        gLINT.intialBtWidth = roundedTbit;

                        /* Transition to positive-edge capture state to complete SYNC validation */
                        gLIN.state = LIN_RX_STATE_SYNC_FIELD_POS_EDGE;
                        /* Mark auto-baud calibration as complete for this frame */
                        gLINT.autoBaud = true;                        
                    }
                    else
                    {
                        /* Auto-baud is disabled but baud rate mismatch detected: report error and reset */
                        LIN_SetError(&gLIN, LIN_ERROR_HEADER);
                        gLIN.errorResetScenario = true;
                        gLIN.state = LIN_STATE_RESET_TRANSITION;
                        LIN_ResetState(LIN_0_INST, &gLIN);
                    }
                }
                else
                {
                    /* All measurements within tolerance: accept current baud rate */
                    gLINT.autoBaud = true;
                    /* Advance to positive-edge state to confirm SYNC field end */
                    gLIN.state = LIN_RX_STATE_SYNC_FIELD_POS_EDGE;
                }
                /* Reset all SYNC measurement accumulators for the next frame */
                gLINT.syncErrCnt = 0;
                gLINT.numCyclesPos = 0;
                gLINT.numCyclesNeg = 0;
                /* Mark that at least one successful SYNC validation has completed */
                gLINT.firstSyncDone = true;
            }
            else
            {
                /* More SYNC cycles to measure: transition to positive-edge capture */
                gLIN.state = LIN_RX_STATE_SYNC_FIELD_POS_EDGE;
            }
        }
        /* Negative edge during active data or TX phase: arm rising-edge for stop-bit check */
        else if(gLIN.state == LIN_RX_STATE_SYNC || gLIN.state == LIN_RX_STATE_ID || 
                gLIN.state == LIN_RX_STATE_DATA || gLIN.state == LIN_RX_STATE_CHECKSUM ||
                gLIN.state == LIN_TX_STATE_SYNC || gLIN.state == LIN_TX_STATE_PID ||
                gLIN.state == LIN_TX_STATE_DATA || gLIN.state == LIN_TX_STATE_CHECKSUM )
        {
            /* Disable and clear negative-edge interrupt; rising-edge will now monitor stop bit */
            DL_UART_disableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
            DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_RXD_NEG_EDGE);
            /* Clear any stale rising-edge flag before enabling the interrupt */
            DL_UART_clearInterruptStatus(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);
            /* Enable rising-edge interrupt to verify the stop bit of the current byte */
            DL_UART_enableInterrupt(LIN_0_INST, DL_UART_EXTEND_INTERRUPT_LIN_RISING_EDGE);

            /* Reset stop-bit error and positive-edge detection flags for this byte */
            gLIN.checkStopBitErrorOccured = false;
            gLIN.checkPosEdgeDetected = false;

            /* Stop and reconfigure counter to measure from this falling edge to the stop bit */
            DL_UART_disableLINCounter(LIN_0_INST);
            /* Set compare to 9.5 bit times: expected stop-bit position from this start bit */
            DL_UART_setLINCounterCompareValue(LIN_0_INST,(uint16_t)(( 19*gLINT.intialBtWidth) >> 1));
            /* Mark that inter-byte timeout is no longer active (stop-bit check is now active) */
            gLIN.timeoutEnabledForInterByte = false;
            DL_UART_setLINCounterValue(LIN_0_INST, 0);
            /* Restart counter to measure time to the expected stop bit */
            DL_UART_enableLINCounter(LIN_0_INST);
        }
        else 
        {
        /* Negative edge in an unrelated state: no action required */
        }
        break;

    /* No other interrupt sources are handled */
    default:
        break;
    }
}