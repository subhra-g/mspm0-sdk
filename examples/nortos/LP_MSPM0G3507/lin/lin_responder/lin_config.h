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

#ifndef LIN_CONFIG_H_
#define LIN_CONFIG_H_

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>
#include "ti_msp_dl_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Enable auto-sleep on bus inactivity */
#define AUTO_SLEEP_ENABLED              (1)

/* Auto-baud rate detection */
#define AUTO_BAUD_ENABLED               (true)

#define DELAY_1MS                       (CPUCLK_FREQ / 1000)

/* UART LIN responder value for the number of messages */
#define LIN_RESPONDER_NUM_MSGS          (3U)

/* Max data buffer size for LIN RX and TX */
#define LIN_DATA_MAX_BUFFER_SIZE        (8)

/* Maximum break field duration in bit times - 27.6 (for timeout detection) */
#define LIN_TBIT_BREAK_FIELD_MAX        (28)

/* Minimum break field duration in bit times (for timeout detection) */
#define LIN_TBIT_BREAK_FIELD_MIN        (13)

/* Minimum break field duration in cycles */
#define LIN_BREAK_FIELD_MIN_CYCLES      (15200)

/* UART LIN value for the number of cycles in a sync validation */
#define LIN_RESPONDER_SYNC_CYCLES       (4)

/* UART LIN value for the message not found */
#define LIN_MESSAGE_NOT_FOUND           (0xFFU)

/* UART LIN value for the sync byte */
#define LIN_SYNC_BYTE                   (0x55U)

/* Number of delay cycles between PID STOP bit and data transmission START bit */
#define LIN_RESPONSE_LAPSE              (LIN_0_INST_FREQUENCY / (2 * LIN_0_BAUD_RATE))

/* LFCLK frequency for timeout calculation */
#define LIN_LFCLK_FREQ                  (32768U)

/* Number of byte times for timeout (3 byte times provides good margin) */
#define LIN_TIMEOUT_BYTE_TIMES          (3U)

/* Baud-rate dependent timeout */
#define LIN_TIMEOUT_TICKS_DEFAULT       ((LIN_TIMEOUT_BYTE_TIMES * 10U * LIN_LFCLK_FREQ) / LIN_0_BAUD_RATE)

/* Macro to calculate timeout ticks for a given baud rate */
#define LIN_CALC_TIMEOUT_TICKS(baud)    ((LIN_TIMEOUT_BYTE_TIMES * 10U * LIN_LFCLK_FREQ) / (baud))

/* Auto-baud tolerance threshold */
#define AUTO_BAUD_THRESHOLD             (3U)

/* Auto-baud configuration delay */
#define AUTO_BAUD_CONFIG_DELAY          (10U)

/* Auto-baud detection percentage thresholds */
#define LIN_AUTO_BAUD_MAX               (105)
#define LIN_AUTO_BAUD_MIN               (95)

/* Supported baud rate range for auto-baud detection */
#define LIN_MAX_SUPPORTED_BAUD          (20000U)
#define LIN_MIN_SUPPORTED_BAUD          (1000U)

#define LIN_DEFAULT_BREAK_CMP           (15200)
#define LIN_BREAK_COMPARE_MIN           (LIN_TBIT_BREAK_FIELD_MIN * (LIN_0_INST_FREQUENCY/LIN_MAX_SUPPORTED_BAUD))
#define LIN_BREAK_COMPARE_MAX           (LIN_TBIT_BREAK_FIELD_MAX * (LIN_0_INST_FREQUENCY/LIN_MIN_SUPPORTED_BAUD))

#define LIN_FRAME_ID_MIN_SIGNAL         (0x00U)
#define LIN_FRAME_ID_MAX_SIGNAL         (0x3BU)
#define LIN_FRAME_ID_COMMANDER_REQ      (0x3CU)
#define LIN_FRAME_ID_RESPONDER_RESP     (0x3DU)
#define LIN_FRAME_ID_RESERVED_1         (0x3EU)
#define LIN_FRAME_ID_RESERVED_2         (0x3FU)

/* Wake-up pulse cycles (250us) */
#define LIN_WAKEUP_PULSE_US             (300U)
#define LIN_US_IN_SECS                  (1000000U)
#define LIN_CYCLES_PER_US               (LIN_0_INST_FREQUENCY / LIN_US_IN_SECS)
#define LIN_WAKEUP_PULSE_CYCLES         (LIN_WAKEUP_PULSE_US * LIN_CYCLES_PER_US)

/* Wake-up compare value for break detection during sleep */
#define LIN_WAKEUP_COMPARE_US           (250U)
#define LIN_WAKEUP_COMPARE_VALUE        (LIN_WAKEUP_COMPARE_US * LIN_CYCLES_PER_US)

#define LIN_SLEEP_CMD_BYTE1             (0x00U)
#define LIN_SLEEP_CMD_FILL              (0xFFU)

#define LIN_MAX_CNTR_VAL                (65535)
#define LIN_MAX_BREAK_MULTIPLIER        (14)

#if defined(__MCU_HAS_UNICOMMUART__)
/* UNICOMM peripheral type and register access */
typedef UNICOMM_Inst_Regs *LIN_Peripheral_t;

#else
/* UART Extended peripheral type and register access */
typedef UART_Regs *LIN_Peripheral_t;
#endif

/*! @enum LIN_STATE - Protocol state machine states */
typedef enum {
    LIN_STATE_WAIT_FOR_BREAK = 0,
    LIN_STATE_BREAK_FIELD,
    LIN_STATE_SYNC_FIELD_NEG_EDGE,
    LIN_STATE_SYNC_FIELD_POS_EDGE,
    LIN_STATE_PID_FIELD,
    LIN_STATE_DATA_FIELD
} LIN_STATE;

/*! @enum LIN_RX_STATE */
typedef enum {
    LIN_RX_STATE_ID = 0,
    LIN_RX_STATE_DATA,
    LIN_RX_STATE_CHECKSUM,
} LIN_RX_STATE;

/*! @enum LIN_CHECKSUM_TYPE - Checksum calculation type per ISO 17987-3 */
typedef enum {
    /* Data only (for diagnostic frames 0x3C, 0x3D) */
    LIN_CHECKSUM_CLASSIC = 0,
    /* Data + PID (for signal frames) */
    LIN_CHECKSUM_ENHANCED
} LIN_CHECKSUM_TYPE;

/*! @enum LIN_DIRECTION - Frame direction for responder */
typedef enum {
    /* Responder sends the response */
    LIN_DIRECTION_PUBLISH = 0,
    /* Responder receives the response */
    LIN_DIRECTION_SUBSCRIBE,
} LIN_DIRECTION;

/*! @enum LIN_ERROR - Error codes for diagnostics */
typedef enum {
    LIN_ERROR_NO_ERROR = 0,
    LIN_ERROR_BREAK_SHORT,
    LIN_ERROR_BREAK_LONG,
    LIN_ERROR_SYNC,
    LIN_ERROR_BIT,
    LIN_ERROR_CHECKSUM,
    LIN_ERROR_FRAMING,
    LIN_ERROR_PID_PARITY,
    LIN_ERROR_NO_RESPONSE,
    LIN_ERROR_INCOMPLETE_RESPONSE
} LIN_ERROR;

/*! @enum LIN_NODE_STATE - Node operational state */
typedef enum {
    LIN_NODE_STATE_BUS_SLEEP = 0,
    LIN_NODE_STATE_OPERATIONAL
} LIN_NODE_STATE;

/** Union for checksum calculation */
typedef union {
    uint16_t word;
    uint8_t byte[2];
} LIN_word_t;

/** Callback function pointer */
typedef void (*LIN_function_ptr_t)(void);

/**
 * @brief Frame configuration record
 */
typedef struct {
    /* 6-bit frame ID */
    uint8_t msgID;
    /* Protected ID (with parity) */
    uint8_t msgPID;
    /* Data field length (1-8 bytes) */
    uint8_t msgSize;
    /* CLASSIC or ENHANCED */
    LIN_CHECKSUM_TYPE checksumType;
    /* PUBLISH/SUBSCRIBE/NONE */
    LIN_DIRECTION direction;
    /* RX callback (NULL for PUBLISH) */
    LIN_function_ptr_t callbackFunction;
} LIN_table_record_t;

/** Sync bit timing measurement */
typedef struct {
    uint16_t negEdge;
    uint16_t posEdge;
} LIN_sync_bits_t;

/**
 * @brief LIN timing and auto-baud rate context
 *
 * Contains all timing-related measurements and auto-baud state.
 */
typedef struct {
    /* Break field timing */
    /* Measured break field width */
    uint32_t brkW;
    /* Break field compare value */
    uint16_t brkCmp;

    /* Sync field measurements */
    /* Sync start bit flag */
    bool syncStart;
    /* Bit timing measurements */
    LIN_sync_bits_t syncBt[LIN_RESPONDER_SYNC_CYCLES];
    /* Valid sync cycles */
    uint8_t syncOkCnt;
    /* Failed sync cycles */
    uint8_t syncErrCnt;

    /* Bit time calculations */
    /* Sum of measured bit times */
    uint16_t btSum;
    /* Average bit time */
    uint16_t btAvg;
    /* Current bit time width */
    uint16_t btWidth;
    /* Minimum acceptable bit width */
    uint16_t btWMin;
    /* Maximum acceptable bit width */
    uint16_t btWMax;

    /* Baud rate tracking */
    /* Measured baud rate */
    uint16_t brMeas;
    /* Previous baud rate */
    uint16_t brPrev;
    /* Current baud rate */
    uint16_t brCurr;

    /* Auto-baud state */
    /* Auto-baud calibration active */
    bool autoBaud;

    /* Response timing */
    /* Delay from PID to response */
    uint16_t respDelay;

    /* Timeout ticks */
    /* RX timeout in LFCLK ticks */
    uint16_t timeoutTicks;
} Lin_timing_t;

/**
 * @brief LIN transmit/receive context
 *
 * Main structure containing the LIN state machine, buffers, and configuration.
 */
typedef struct {
    /* State machine */
    volatile LIN_STATE state;

    /* Message table */
    LIN_table_record_t *respMsgTbl;
    uint8_t msgTblIdx;

    /* Receive context */
    volatile uint8_t rxIdx;
    uint8_t rxBuf[LIN_DATA_MAX_BUFFER_SIZE];

    /* Transmit context */
    volatile uint8_t txIdx;
    uint8_t txBuf[LIN_DATA_MAX_BUFFER_SIZE];

    /* Checksum */
    LIN_word_t chkSum;

    /* Current PID for checksum calculation */
    uint8_t currentPID;

    /* Timing reference */
    volatile Lin_timing_t *timing;

    /* Node state */
    LIN_NODE_STATE nodeState;

    /* Error tracking */
    LIN_ERROR lastError;
    uint32_t errorCount;
    uint32_t successCount;
    bool responseError;

    /* Bus inactivity */
    uint32_t busInactivityCounter;
    bool autoSleepEnabled;

    /* Diagnostic frame flag */
    bool receivingDiagnosticFrame;
    uint8_t expectedDataLength;
} Lin_TxRxCtx_t;

extern LIN_table_record_t responderMessageTable[LIN_RESPONDER_NUM_MSGS];

/* Global LIN transmit/receive context */
extern Lin_TxRxCtx_t gLIN;

/* Global LIN timing context */
extern volatile Lin_timing_t gLINT;

/**
 * @brief Calculate protected identifier (ID + parity)
 * @param frameID 6-bit frame identifier
 * @return 8-bit PID
 */
uint8_t LIN_calculatePID(uint8_t frameID);

/**
 * @brief Validate PID parity
 * @param pid Protected identifier to validate
 * @return true if parity is correct
 */
bool LIN_validatePID(uint8_t pid);

/**
 * @brief Extract frame ID from PID
 * @param pid Protected identifier
 * @return 6-bit frame ID
 */
uint8_t LIN_getFrameIDFromPID(uint8_t pid);

/**
 * @brief Calculate LIN checksum
 * @param data Pointer to data bytes
 * @param length Number of data bytes
 * @param pid Protected identifier
 * @param type CLASSIC or ENHANCED checksum type
 * @return Calculated checksum byte
 */
uint8_t LIN_calculateChecksum(uint8_t *data, uint8_t length, uint8_t pid, LIN_CHECKSUM_TYPE type);

/**
 * @brief Reset LIN state machine to wait for break
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 * @param lin Pointer to LIN context structure
 */
void LIN_Responder_resetState(LIN_Peripheral_t peripheral, void *timer, Lin_TxRxCtx_t *lin);

/**
 * @brief Prepare for next frame after successful completion
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 */
void LIN_prepareForNextFrame(LIN_Peripheral_t peripheral, void *timer);

/**
 * @brief Configure LIN counter for sync field measurement
 * @param peripheral Pointer to LIN peripheral instance
 */
void LIN_switchCounterToSyncMode(LIN_Peripheral_t peripheral);

/**
 * @brief Configure LIN counter for break field detection
 * @param peripheral Pointer to LIN peripheral instance
 */
void LIN_switchCounterToBreakMode(LIN_Peripheral_t peripheral);

/**
 * @brief Process received Protected Identifier (PID)
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 * @param rxByte Received PID byte
 * @param lin Pointer to LIN context structure
 */
void LIN_Responder_receivePID(LIN_Peripheral_t peripheral, void *timer, uint8_t rxByte, Lin_TxRxCtx_t *lin);

/**
 * @brief Receive data bytes and validate checksum
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 * @param rxByte Received data byte
 * @param lin Pointer to LIN context structure
 */
void LIN_Responder_receiveMessage(LIN_Peripheral_t peripheral, void *timer, uint8_t rxByte, Lin_TxRxCtx_t *lin);

/**
 * @brief Transmit data bytes and checksum (TX interrupt mode)
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 * @param lin Pointer to LIN context structure
 */
void LIN_Responder_transmitMessage(LIN_Peripheral_t peripheral, void *timer, Lin_TxRxCtx_t *lin);

/**
 * @brief Set error status
 * @param lin Pointer to LIN context structure
 * @param error Error type to set
 */
void LIN_Responder_setError(Lin_TxRxCtx_t *lin, LIN_ERROR error);

/**
 * @brief Clear last error
 * @param lin Pointer to LIN context structure
 */
void LIN_Responder_clearError(Lin_TxRxCtx_t *lin);

/**
 * @brief Send wake-up signal on LIN bus
 * @param peripheral Pointer to LIN peripheral instance
 */
void LIN_Responder_sendWakeup(LIN_Peripheral_t peripheral);

/**
 * @brief Enter bus sleep mode
 * @param peripheral Pointer to LIN peripheral instance
 * @param lin Pointer to LIN context structure
 */
void LIN_Responder_enterSleep(LIN_Peripheral_t peripheral, Lin_TxRxCtx_t *lin);

/**
 * @brief Wake up from bus sleep
 * @param lin Pointer to LIN context structure
 */
void LIN_Responder_wakeup(Lin_TxRxCtx_t *lin);

/**
 * @brief Check if bus is sleeping
 * @param lin Pointer to LIN context structure
 * @return true if in sleep mode
 */
bool LIN_Responder_isBusSleeping(Lin_TxRxCtx_t *lin);

/**
 * @brief Check if data contains sleep command
 * @param data Pointer to received data buffer
 * @return true if sleep command detected
 */
bool LIN_Responder_checkSleepCommand(uint8_t *data);

/**
 * @brief Called after successful TX transmission
 */
extern void LIN_processMessage_Tx(void);

/**
 * @brief Called after successful wake-up
 */
extern void LIN_processWakeUp(void);

/**
 * @brief Called after successful sleep
 */
extern void LIN_processSleep(void);

/**
 * @brief Called when an error is detected
 * @param error The error type that occurred
 */
extern void LIN_processError(LIN_ERROR error);

#endif /* LIN_CONFIG_H_ */
