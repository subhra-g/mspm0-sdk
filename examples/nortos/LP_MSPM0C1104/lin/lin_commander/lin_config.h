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

/* Number of messages in the commander message table */
#define LIN_COMMANDER_NUM_MSGS          (4U)

/* Max data buffer size for LIN RX and TX */
#define LIN_DATA_MAX_BUFFER_SIZE        (8)

/* UART LIN value for the sync byte */
#define LIN_SYNC_BYTE                   (0x55U)

/*
 * Break field duration in CPU cycles.
 * LIN 2.x requires minimum 13 Tbit for break field.
 * Calculated for configured baud rate at CPU frequency.
 */
#define LIN_BREAK_LENGTH                ((uint32_t)(LIN_0_TBIT_WIDTH * 13))

/* LFCLK frequency for timeout calculation */
#define LIN_LFCLK_FREQ                  (32768U)

/* Number of byte times for timeout (3 byte times provides good margin) */
#define LIN_TIMEOUT_BYTE_TIMES          (3U)

/* Baud-rate dependent timeout */
#define LIN_TIMEOUT_TICKS_DEFAULT       ((LIN_TIMEOUT_BYTE_TIMES * 10U * LIN_LFCLK_FREQ) / LIN_0_BAUD_RATE)

/* Frame timeout period (timer reload value for frame timeout detection) */
#define TIMEOUT                         (LIN_TIMEOUT_TICKS_DEFAULT)

#define LIN_FRAME_ID_MIN_SIGNAL         (0x00U)
#define LIN_FRAME_ID_MAX_SIGNAL         (0x3BU)
#define LIN_FRAME_ID_COMMANDER_REQ      (0x3CU)
#define LIN_FRAME_ID_RESPONDER_RESP     (0x3DU)
#define LIN_FRAME_ID_RESERVED_1         (0x3EU)
#define LIN_FRAME_ID_RESERVED_2         (0x3FU)

/* Wake-up pulse cycles (300us) */
#define LIN_WAKEUP_PULSE_US             (300U)
#define LIN_US_IN_SECS                  (1000000U)
#define LIN_CYCLES_PER_US               (LIN_0_INST_FREQUENCY / LIN_US_IN_SECS)
#define LIN_WAKEUP_PULSE_CYCLES         (LIN_WAKEUP_PULSE_US * LIN_CYCLES_PER_US)

#define LIN_SLEEP_CMD_BYTE1             (0x00U)
#define LIN_SLEEP_CMD_FILL              (0xFFU)

/* Delay between operations in milliseconds */
#define OPERATION_DELAY_MS              (500U)

/* Delay cycles calculation */
#define DELAY_1MS                       (CPUCLK_FREQ / 1000)
#define OPERATION_DELAY_CYCLES          (OPERATION_DELAY_MS * DELAY_1MS)

#if defined(__MCU_HAS_UNICOMMUART__)
/* UNICOMM peripheral type and register access */
typedef UNICOMM_Inst_Regs *LIN_Peripheral_t;

#else
/* UART Extended peripheral type and register access */
typedef UART_Regs *LIN_Peripheral_t;
#endif

#define MSG_IDX_PUBLISH (0U)
#define MSG_IDX_SUBSCRIBE (1U)
#define MSG_IDX_R2R_1 (2U)
#define MSG_IDX_R2R_2 (3U)

typedef enum {
  OP_STATE_PUBLISH = 0,
  OP_STATE_SUBSCRIBE,
  OP_STATE_RESP_TO_RESP_1,
  OP_STATE_RESP_TO_RESP_2,
  OP_STATE_SLEEP,
  OP_STATE_WAKEUP,
  OP_STATE_SYNC_ERROR,
  OP_STATE_CHKSUM_ERROR,
  OP_STATE_PID_PARITY_ERROR,
  OP_STATE_COM_NO_RES_ERROR,
  OP_STATE_COM_INCMPLT_RES_ERROR,
  OP_STATE_RES_NO_RES_ERROR,
  OP_STATE_RES_INCMPLT_RES_ERROR,
  OP_STATE_MAX
} OperationState_t;

/*! @enum LIN_RX_STATE - Commander receive state machine states */
typedef enum {
    LIN_RX_STATE_ID = 0,
    LIN_RX_STATE_DATA,
    LIN_RX_STATE_CHECKSUM,
} LIN_RX_STATE;

/*! @enum LIN_STATE - Protocol state machine states */
typedef enum {
    LIN_STATE_WAIT_FOR_BREAK = 0,
    LIN_STATE_BREAK_FIELD,
    LIN_STATE_SYNC_FIELD_NEG_EDGE,
    LIN_STATE_SYNC_FIELD_POS_EDGE,
    LIN_STATE_PID_FIELD,
    LIN_STATE_DATA_FIELD
} LIN_STATE;

/*! @enum LIN_CHECKSUM_TYPE - Checksum calculation type per ISO 17987-3 */
typedef enum {
    /* Data only (for diagnostic frames 0x3C, 0x3D) */
    LIN_CHECKSUM_CLASSIC = 0,
    /* Data + PID (for signal frames) */
    LIN_CHECKSUM_ENHANCED
} LIN_CHECKSUM_TYPE;

/*! @enum LIN_DIRECTION - Frame direction for commander */
typedef enum {
    /* Commander sends data */
    LIN_DIRECTION_PUBLISH = 0,
    /* Commander receives data */
    LIN_DIRECTION_SUBSCRIBE,
    /* Header only (RESPONDER-to-RESPONDER) */
    LIN_DIRECTION_IGNORE
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

/**
 * @brief LIN Commander transmit/receive context
 *
 * Main structure containing the LIN state machine, buffers, and configuration.
 */
typedef struct {
    /* State machines */
    volatile LIN_RX_STATE rxState;
    volatile LIN_STATE state;

    /* Message table */
    LIN_table_record_t *cmdMsgTbl;
    uint8_t msgTblIdx;

    /* Receive context */
    volatile uint8_t rxIdx;
    uint8_t rxBuf[LIN_DATA_MAX_BUFFER_SIZE];

    /* Transmit context */
    volatile uint8_t txIdx;
    uint8_t txBuf[LIN_DATA_MAX_BUFFER_SIZE];

    /* Checksum */
    LIN_word_t chkSum;
    LIN_word_t txChkSum;

    /* Current PID for checksum calculation */
    uint8_t currentPID;

    /* Node state */
    LIN_NODE_STATE nodeState;

    /* Error tracking */
    LIN_ERROR lastError;
    uint32_t errorCount;
    uint32_t successCount;

    volatile bool transactionCompleted;
} Lin_TxRxCtx_t;

extern LIN_table_record_t commanderMessageTable[LIN_COMMANDER_NUM_MSGS];

/* Global LIN commander context */
extern Lin_TxRxCtx_t gLIN;

/**
 * @brief Calculate protected identifier (ID + parity)
 * @param frameID 6-bit frame identifier
 * @return 8-bit PID
 */
extern uint8_t LIN_calculatePID(uint8_t frameID);

/**
 * @brief Validate PID parity
 * @param pid Protected identifier to validate
 * @return true if parity is correct
 */
extern bool LIN_validatePID(uint8_t pid);

/**
 * @brief Extract frame ID from PID
 * @param pid Protected identifier
 * @return 6-bit frame ID
 */
extern uint8_t LIN_getFrameIDFromPID(uint8_t pid);

/**
 * @brief Calculate LIN checksum
 * @param data Pointer to data bytes
 * @param length Number of data bytes
 * @param pid Protected identifier
 * @param type CLASSIC or ENHANCED checksum type
 * @return Calculated checksum byte
 */
extern uint8_t LIN_calculateChecksum(uint8_t *data, uint8_t length, uint8_t pid, LIN_CHECKSUM_TYPE type);

/**
 * @brief Send LIN frame (header + optional data)
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 * @param tableIndex Index into message table
 * @param txBuffer Pointer to transmit data buffer
 * @param messageTable Pointer to commander message table
 */
extern void LIN_Commander_sendFrame(LIN_Peripheral_t peripheral, void *timer, uint8_t tableIndex, uint8_t *txBuffer, LIN_table_record_t *messageTable);

/**
 * @brief Receive data bytes and validate checksum
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 * @param rxByte Received data byte
 * @param rxBuffer Pointer to receive buffer
 * @param messageTable Pointer to commander message table
 */
extern void LIN_Commander_receiveMessage(LIN_Peripheral_t peripheral, void *timer, uint8_t rxByte, uint8_t *rxBuffer, LIN_table_record_t *messageTable);

/**
 * @brief Transmit data bytes and checksum (TX interrupt mode)
 * @param peripheral Pointer to LIN peripheral instance
 * @param txBuffer Pointer to transmit data buffer
 * @param messageTable Pointer to commander message table
 */
extern void LIN_Commander_transmitMessage(LIN_Peripheral_t peripheral, uint8_t *txBuffer, LIN_table_record_t *messageTable);

/**
 * @brief Handle RX timeout
 * @param peripheral Pointer to LIN peripheral instance
 * @param timer Pointer to timer instance
 */
extern void LIN_Commander_handleTimeout(LIN_Peripheral_t peripheral, void *timer);

/**
 * @brief Set error status
 * @param lin Pointer to LIN commander context
 * @param error Error type to set
 */
extern void LIN_Commander_setError(Lin_TxRxCtx_t *lin, LIN_ERROR error);

/**
 * @brief Clear last error
 * @param lin Pointer to LIN commander context
 */
extern void LIN_Commander_clearError(Lin_TxRxCtx_t *lin);

/**
 * @brief Send go-to-sleep command (COMMANDER Request frame 0x3C)
 * @param peripheral Pointer to LIN peripheral instance
 */
extern void LIN_Commander_sendGoToSleep(LIN_Peripheral_t peripheral);

/**
 * @brief Send wake-up signal on LIN bus
 * @param peripheral Pointer to LIN peripheral instance
 */
extern void LIN_Commander_sendWakeup(LIN_Peripheral_t peripheral);

/**
 * @brief Check if bus is sleeping
 * @param lin Pointer to LIN commander context
 * @return true if in sleep mode
 */
extern bool LIN_Commander_isBusSleeping(Lin_TxRxCtx_t *lin);

/**
 * @brief Called when an error is detected
 * @param error The error type that occurred
 */
extern void LIN_processError(LIN_ERROR error);

#endif /* LIN_CONFIG_H_ */
