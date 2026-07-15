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

/* Header guard: prevents multiple inclusions of this header file */
#ifndef LIN_CONFIG_H_
#define LIN_CONFIG_H_

/* MSP device register definitions */
#include <ti/devices/msp/msp.h>
/* TI DriverLib API for peripheral control */
#include <ti/driverlib/driverlib.h>
/* Core M0+ CPU support functions */
#include <ti/driverlib/m0p/dl_core.h>
/* Auto-generated SysConfig peripheral initialization header */
#include "ti_msp_dl_config.h"
/* Standard boolean type */
#include <stdbool.h>
/* Standard fixed-width integer types */
#include <stdint.h>
/* memcpy and string utilities */
#include <string.h>

/* Number of CPU cycles for timeout*/
#define LIN_TIMEOUT_DURATION            (65535)

/* Number of CPU cycles to delay between operations for settling time */
#define OPERATION_DELAY_CYCLES          (1000)

/* Auto-baud rate detection: set to true to enable runtime baud rate measurement from SYNC field */
#define AUTO_BAUD_ENABLED               (true)

/* Number of CPU clock cycles equivalent to 1 millisecond delay */
#define DELAY_1MS                       (CPUCLK_FREQ / 1000)

/* Number of LIN frames handled by the responder node message table */
#define LIN_RESPONDER_NUM_MSGS          (3U)

/* Maximum number of data bytes in a single LIN frame (RX or TX buffer size) */
#define LIN_DATA_MAX_BUFFER_SIZE        (8)

/* Number of LIN frames in the commander node message table */
#define LIN_COMMANDER_NUM_MSGS          (4U)

/* Minimum valid signal frame ID (0x00) */
#define LIN_FRAME_ID_MIN_SIGNAL         (0x00U)
/* Maximum valid signal frame ID (0x3B) */
#define LIN_FRAME_ID_MAX_SIGNAL         (0x3BU)
/* Frame ID for commander diagnostic request (master request frame) */
#define LIN_FRAME_ID_COMMANDER_REQ      (0x3CU)
/* Frame ID for responder diagnostic response (slave response frame) */
#define LIN_FRAME_ID_RESPONDER_RESP     (0x3DU)
/* Reserved frame ID 1 - not used for normal communication */
#define LIN_FRAME_ID_RESERVED_1         (0x3EU)
/* Reserved frame ID 2 - not used for normal communication */
#define LIN_FRAME_ID_RESERVED_2         (0x3FU)

/* Index into the message table for the publish (commander TX) frame */
#define MSG_IDX_PUBLISH (0U)
/* Index into the message table for the subscribe (commander RX) frame */
#define MSG_IDX_SUBSCRIBE (1U)
/* Index into the message table for responder-to-responder frame 1 */
#define MSG_IDX_R2R_1 (2U)
/* Index into the message table for responder-to-responder frame 2 */
#define MSG_IDX_R2R_2 (3U)

/* Application-level operation state machine used in the example main loop */
typedef enum {
  /* Commander is publishing (transmitting) data to the bus */
  OP_STATE_PUBLISH = 0,
  /* Commander is subscribing (receiving) data from the bus */
  OP_STATE_SUBSCRIBE,
  /* Responder-to-responder communication, first phase */
  OP_STATE_RESP_TO_RESP_1,
  /* Responder-to-responder communication, second phase */
  OP_STATE_RESP_TO_RESP_2,
  /* Node is entering or in bus sleep mode */
  OP_STATE_SLEEP,
  /* Node is performing or completed a wakeup sequence */
  OP_STATE_WAKEUP,
  /* A checksum error was detected on a received frame */
  OP_STATE_CHKSUM_ERROR,
  /* Incomplete response received from responder (partial data) */
  OP_STATE_RES_INCMPLT_RES_ERROR,
  /* Sentinel value: total number of operation states */
  OP_STATE_MAX
} OperationState_t;

/* Number of dominant (low) bits transmitted for a LIN break field */
#define LIN_BREAK_BIT_COUNT (13U)

/* Maximum break field duration in bit times used for upper-bound timeout detection (27.6 rounded up) */
#define LIN_TBIT_COUNTER_COEFFICIENT_MAX        (28)

/* Minimum break field duration in bit times used for lower-bound timeout detection */
#define LIN_TBIT_COUNTER_COEFFICIENT_MIN        (11)

/* Number of SYNC field bit-pair measurements used for baud rate validation */
#define LIN_RESPONDER_SYNC_CYCLES       (4)

/* Standard LIN SYNC byte value (alternating bits, 0x55) used for baud rate measurement */
#define LIN_SYNC_BYTE                   (0x55U)

/* Half-bit delay in peripheral clock cycles between PID stop bit and first response data start bit */
#define LIN_RESPONSE_LAPSE              (LIN_0_INST_FREQUENCY / (2 * LIN_0_BAUD_RATE))

/* Frequency of the low-frequency clock (LFCLK) in Hz, used for inter-byte timeout tick calculation */
#define LIN_LFCLK_FREQ                  (32768U)

/* Number of byte times (each = 10 bit times) to allow as inter-byte timeout margin */
#define LIN_TIMEOUT_BYTE_TIMES          (3U)

/* Default inter-byte timeout in LFCLK ticks, calculated from nominal baud rate */
#define LIN_TIMEOUT_TICKS_DEFAULT       ((LIN_TIMEOUT_BYTE_TIMES * 10U * LIN_LFCLK_FREQ) / LIN_0_BAUD_RATE)

/* Macro to compute inter-byte timeout in LFCLK ticks for a given baud rate */
#define LIN_CALC_TIMEOUT_TICKS(baud)    ((LIN_TIMEOUT_BYTE_TIMES * 10U * LIN_LFCLK_FREQ) / (baud))

/* Number of SYNC measurement cycles that may deviate before triggering auto-baud recalibration */
#define AUTO_BAUD_THRESHOLD             (3U)

/* Enable automatic bus sleep after detecting a go-to-sleep command */
#define AUTO_SLEEP_ENABLED              (true)

/* Number of peripheral clock cycles to wait after reconfiguring UART for new baud rate */
#define AUTO_BAUD_CONFIG_DELAY          (10U)

/* Upper tolerance percentage for auto-baud measured bit time (5% above nominal) */
#define LIN_AUTO_BAUD_MAX               (105)
/* Lower tolerance percentage for auto-baud measured bit time (5% below nominal) */
#define LIN_AUTO_BAUD_MIN               (95)

/* Maximum baud rate supported by auto-baud detection (20 kbps per LIN spec) */
#define LIN_MAX_SUPPORTED_BAUD          (20000U)
/* Minimum baud rate supported by auto-baud detection (1 kbps per LIN spec) */
#define LIN_MIN_SUPPORTED_BAUD          (1000U)

/* Minimum break field counter compare value: minimum valid bit times at maximum baud rate */
#define LIN_BREAK_COMPARE_MIN_CYCLES    (LIN_TBIT_COUNTER_COEFFICIENT_MIN * (LIN_0_INST_FREQUENCY/LIN_MAX_SUPPORTED_BAUD))
/* Maximum break field counter compare value: maximum valid bit times at minimum baud rate */
#define LIN_BREAK_COMPARE_MAX_CYCLES    (LIN_TBIT_COUNTER_COEFFICIENT_MAX * (LIN_0_INST_FREQUENCY/LIN_MIN_SUPPORTED_BAUD))

/* Duplicate definitions retained for compatibility - minimum valid signal frame ID */
#define LIN_FRAME_ID_MIN_SIGNAL         (0x00U)
/* Duplicate definitions retained for compatibility - maximum valid signal frame ID */
#define LIN_FRAME_ID_MAX_SIGNAL         (0x3BU)
/* Duplicate definitions retained for compatibility - commander diagnostic request ID */
#define LIN_FRAME_ID_COMMANDER_REQ      (0x3CU)
/* Duplicate definitions retained for compatibility - responder diagnostic response ID */
#define LIN_FRAME_ID_RESPONDER_RESP     (0x3DU)
/* Duplicate definitions retained for compatibility - reserved frame ID 1 */
#define LIN_FRAME_ID_RESERVED_1         (0x3EU)
/* Duplicate definitions retained for compatibility - reserved frame ID 2 */
#define LIN_FRAME_ID_RESERVED_2         (0x3FU)

/* Minimum wakeup pulse duration in microseconds per LIN specification */
#define LIN_WAKE_UP_MIN_TIME_IN_US      (250U)
/* Maximum wakeup pulse duration in microseconds per LIN specification */
#define LIN_WAKE_UP_MAX_TIME_IN_US      (5000U)
/* Scaling factor to convert Hz to MHz for cycles-per-microsecond calculation */
#define LIN_SCALE_IN_MHZ                (1000000U)
/* Number of peripheral clock cycles per microsecond */
#define LIN_CYCLES_PER_US               (LIN_0_INST_FREQUENCY / LIN_SCALE_IN_MHZ)
/* Counter compare value corresponding to minimum valid wakeup pulse width */
#define LIN_WAKEUP_COMPARE_VALUE        (LIN_WAKE_UP_MIN_TIME_IN_US * LIN_CYCLES_PER_US)

/* First data byte of the go-to-sleep command frame (must be 0x00) */
#define LIN_SLEEP_CMD_BYTE1             (0x00U)
/* Filler byte for remaining data bytes of the go-to-sleep frame (must be 0xFF) */
#define LIN_SLEEP_CMD_FILL              (0xFFU)

/* Maximum 16-bit counter value (used for counter rollover/limit checks) */
#define LIN_MAX_CNTR_VAL                (65535)

/* Maximum break field length multiplier for timeout boundary calculations */
#define LIN_MAX_BREAK_MULTIPLIER        (14)

/* Maximum number of data bytes per LIN frame per LIN 2.x specification */
#define LIN_MAX_DATA_LENGTH             (8)

/* AUTOSAR SWS_Std_00007: logical high physical voltage level (5V or 3.3V) */
#define STD_HIGH    (1)
/* AUTOSAR SWS_Std_00007: logical low physical voltage level (0V) */
#define STD_LOW     (0)

/* Select peripheral pointer type based on whether the MCU has a UNICOMM UART */
#if defined(__MCU_HAS_UNICOMMUART__)
/* UNICOMM peripheral register pointer type for LIN instance */
typedef UNICOMM_Inst_Regs *LIN_Peripheral_t;

#else
/* Standard UART Extended peripheral register pointer type for LIN instance */
typedef UART_Regs *LIN_Peripheral_t;
#endif

/*! @enum LIN_STATE - Protocol state machine states */
typedef enum 
{
    /*! LIN RX state wait for Break field of tyhe RX packet */
    LIN_RX_STATE_WAIT_FOR_BREAK = 0,
    /*! LIN RX state in Break field of tyhe RX packet */
    LIN_RX_STATE_BREAK_FIELD,
     /*! UART LIN state of waiting for the negative edge of the sync field */
    LIN_RX_STATE_SYNC_FIELD_NEG_EDGE = 2,
    /*! UART LIN state of waiting for the positive edge of the sync field */
    LIN_RX_STATE_SYNC_FIELD_POS_EDGE = 3,
    /*! LIN RX state for the Sync byte of the RX packet */
    LIN_RX_STATE_SYNC,
    /*! LIN RX state for the ID byte of the RX packet */
    LIN_RX_STATE_ID,
    /*! LIN RX state for the data bytes of the RX packet */
    LIN_RX_STATE_DATA,
    /*! LIN RX state for the checksum byte of the RX packet */
    LIN_RX_STATE_CHECKSUM,
    /*! LIN RX state for the RX packet received */
    LIN_RX_STATE_COMPLETE,
    /*! LIN TX state waiting for sendframe api call */
    LIN_TX_STATE_WAIT_FOR_SEND_FRAME,
    /*! LIN TX state for sending Break field */
    LIN_TX_STATE_BREAK,
    /*! LIN TX state for sending Sync byte */
    LIN_TX_STATE_SYNC,
    /*! LIN TX state for sending PID byte */
    LIN_TX_STATE_PID,
    /*! LIN TX state for sending data bytes */
    LIN_TX_STATE_DATA,
    /*! LIN TX state for sending checksum byte */
    LIN_TX_STATE_CHECKSUM,
    /*! LIN TX state for transmission complete */
    LIN_TX_STATE_COMPLETE,
    /*! LIN state to wait for wake-up */
    LIN_STATE_WAIT_FOR_WAKEUP,
    /*! LIN transition state after error */
    LIN_STATE_RESET_TRANSITION
    
} LIN_STATE;

/*! @enum LIN_ERROR - Error codes for diagnostics */
typedef enum {
    /* No error has occurred */
    LIN_ERROR_NO_ERROR = 0,
    /* Break field was shorter than the minimum allowed duration */
    LIN_ERROR_BREAK_SHORT,
    /* Break field was longer than the maximum allowed duration */
    LIN_ERROR_BREAK_LONG,
    /* SYNC byte value or timing did not match expected pattern */
    LIN_ERROR_SYNC,
    /* A bit error was detected during transmission (transmitted != received) */
    LIN_ERROR_BIT,
    /* Received checksum did not match the calculated checksum */
    LIN_ERROR_CHECKSUM,
    /* UART framing error detected (missing or incorrect stop bit) */
    LIN_ERROR_FRAMING,
    /* PID parity bits P0 or P1 do not match the frame ID */
    LIN_ERROR_PID_PARITY,
    /* No response data received after the PID when one was expected */
    LIN_ERROR_NO_RESPONSE,
    /* Response started but did not complete (fewer bytes than expected) */
    LIN_ERROR_INCOMPLETE_RESPONSE,
    /* Stop bit error detected on a response data byte */
    LIN_ERROR_RESP_STOPBIT,
    /* General header error (break, sync, or PID validation failure) */
    LIN_ERROR_HEADER,
    /* Data bit mismatch between transmitted and looped-back received byte */
    LIN_ERROR_RESP_DATABIT
} LIN_ERROR;

/* LIN channel status type indicating the current frame transfer result or channel state */
typedef enum Lin_StatusTag
{
   /* Operation failed or channel is in an undefined state */
   LIN_NOT_OK,
   /* Transmission completed successfully */
   LIN_TX_OK,
   /* Transmission is currently in progress */
   LIN_TX_BUSY,
   /* An error occurred while transmitting the LIN header (break/sync/PID) */
   LIN_TX_HEADER_ERROR,
   /* An error occurred during data transmission */
   LIN_TX_ERROR,
   /* Reception completed successfully with valid checksum */
   LIN_RX_OK,
   /* Reception is currently in progress */
   LIN_RX_BUSY,
   /* Reception completed but checksum validation failed */
   LIN_RX_ERROR,
   /* No response data received from the responder */
   LIN_RX_NO_RESPONSE,
   /* Channel is active and operational (not sleeping) */
   LIN_OPERATIONAL,
   /* Channel is in the bus sleep state */
   LIN_CH_SLEEP
} Lin_StatusType;

/*! @enum LIN_NODE_STATE - Node operational state */
typedef enum Lin_ChannelActivityStatusTag
{
    /* Channel is idle: no frame is currently being transmitted or received */
    LIN_CHANNEL_IDLE,
    /* Channel is busy: a frame transmission or reception is in progress */
    LIN_CHANNEL_BUSY,
} Lin_ChannelActivityStatusType;

/* Union allowing a 16-bit checksum accumulator to be accessed as two individual bytes for carry folding */
typedef union {
    uint16_t word;
    uint8_t byte[2];
} LIN_word_t;

/* Function pointer type for LIN frame RX/TX completion callback with no arguments and no return value */
typedef void (*LIN_function_ptr_t)(void);

/* LIN node role: commander initiates frames; responder reacts to headers */
typedef enum Lin_NodeTag
{
   /* This node acts as the LIN commander (bus master) */
   LIN_COMMANDER,
   /* This node acts as a LIN responder (bus slave) */
   LIN_RESPONDER
} Lin_NodeType;

/* LIN checksum model selection per LIN 2.x specification */
typedef enum Lin_FrameCsModelTag
{
   /* Enhanced checksum: includes PID in checksum calculation (LIN 2.x frames) */
   LIN_ENHANCED_CS,
   /* Classic checksum: covers data bytes only (LIN 1.x frames and diagnostic frames) */
   LIN_CLASSIC_CS
} Lin_FrameCsModelType;

/* Direction of a LIN frame response relative to this node */
typedef enum Lin_FrameResponseTag
{
   /* This node transmits the response data bytes */
   LIN_FRAMERESPONSE_TX,
   /* This node receives the response data bytes */
   LIN_FRAMERESPONSE_RX,
   /* This node neither transmits nor receives the response (header only) */
   LIN_FRAMERESPONSE_IGNORE
} Lin_FrameResponseType;

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
    Lin_FrameCsModelType checksumType;
    /* PUBLISH/SUBSCRIBE/NONE */
    Lin_FrameResponseType direction;
    /* RX callback (NULL for PUBLISH) */
    LIN_function_ptr_t callbackFunction;
} LIN_table_record_t;

/* Captured hardware timer values for a single SYNC field bit-pair (falling and rising edges) */
typedef struct {
    /* Counter value captured at the falling edge of the SYNC bit */
    uint16_t negEdge;
    /* Counter value captured at the rising edge of the SYNC bit */
    uint16_t posEdge;
} LIN_sync_bits_t;

/**
 * @brief LIN timing and auto-baud rate context
 *
 * Contains all timing-related measurements and auto-baud state.
 */
typedef struct {
    /* Break field timing */
    /* Measured break field width in peripheral clock cycles */
    uint32_t brkW;
    /* Hardware counter compare value for minimum valid break field detection */
    uint16_t brkCmp;

    /* Sync field measurements */
    /* Flag set true at the start of SYNC field measurement to indicate first edge expected */
    bool syncStart;
    /* Array of captured falling/rising edge pairs for each of the 4 SYNC bit intervals */
    LIN_sync_bits_t syncBt[LIN_RESPONDER_SYNC_CYCLES];
    /* Count of SYNC bit measurements that passed the tolerance check */
    uint8_t syncOkCnt;
    /* Count of SYNC bit measurements that failed the tolerance check */
    uint8_t syncErrCnt;

    /* Bit time calculations */
    /* Accumulated sum of all measured SYNC bit widths for averaging */
    uint32_t btSum;
    /* Computed average bit time across all valid SYNC measurements */
    uint16_t btAvg;
    /* Current operating bit time in peripheral clock cycles */
    uint32_t btWidth;

    /* Bit time width captured at startup or after last successful auto-baud calibration */
    uint32_t intialBtWidth;

    /* Bit time threshold used to select between btWidth and btWidthLimit for break validation */
    uint32_t btWidthLimit;
    /* Minimum acceptable measured bit time (btWidth * LIN_AUTO_BAUD_MIN / 100) */
    uint16_t btWMin;
    /* Maximum acceptable measured bit time (btWidth * LIN_AUTO_BAUD_MAX / 100) */
    uint16_t btWMax;

    /* Baud rate tracking */
    /* Most recently measured baud rate derived from SYNC field timing */
    uint16_t brMeas;
    /* Baud rate used in the previous frame */
    uint16_t brPrev;
    /* Baud rate currently configured in the UART peripheral */
    uint16_t brCurr;

    /* Auto-baud state */
    /* True while an auto-baud recalibration cycle is in progress */
    bool autoBaud;

    /* Response timing */
    /* Half-bit delay in peripheral clock cycles inserted between PID and first response byte */
    uint16_t respDelay;

    /* Timeout ticks */
    /* Inter-byte timeout value in LFCLK ticks used for the LIN counter compare register */
    uint16_t timeoutTicks;

    /* Number of falling edge captures accumulated during current SYNC measurement pass */
    uint16_t numCyclesNeg;
    /* Number of rising edge captures accumulated during current SYNC measurement pass */
    uint16_t numCyclesPos;
    /* Set to true after the very first successful SYNC field validation completes */
    volatile bool firstSyncDone;

} Lin_timing_t;

/* LIN Protocol Data Unit: holds all fields of a single LIN frame for TX or RX */
typedef struct Lin_PduTag
{
   /* Protected Identifier byte (6-bit ID + 2 parity bits) */
   uint8_t Pid;
   /* Checksum model to apply: LIN_ENHANCED_CS or LIN_CLASSIC_CS */
   Lin_FrameCsModelType Cs;
   /* Frame response direction: TX, RX, or IGNORE */
   Lin_FrameResponseType Drc;
   /* Data length: number of bytes in the data field (1–8) */
   uint8_t Dl;
   /* Current index into the data buffer during byte-by-byte TX or RX */
   uint8_t byteCounter;
   /* Pointer to the application-supplied data buffer */
   uint8_t * SduPtr;
   /* Internal shadow copy of data used during transmission to allow safe loopback checking */
   uint8_t shadowBuffer[LIN_MAX_DATA_LENGTH];
} Lin_PduType;

/* Controls whether SYNC field tolerance validation is applied to every frame or only the first */
typedef enum {
   /* Perform SYNC tolerance check only on the first received frame after reset or sleep */
   LIN_SYNC_CHECK_FIRST_FRAME,
   /* Perform SYNC tolerance check on every received frame */
   LIN_SYNC_CHECK_ALL_FRAMES
} LIN_SYNC_CHECK_FRAME_TYPE;

/* LIN channel network-level power state */
typedef enum Lin_ChannelNetworkStatusTag
{
    /* Channel is fully powered and communicating normally */
    LIN_CHANNEL_OPERATIONAL,
    /* A go-to-sleep command has been sent but sleep state is not yet confirmed */
    LIN_CHANNEL_SLEEP_PENDING,
    /* Channel has entered bus sleep mode */
    LIN_CHANNEL_SLEEP
} Lin_ChannelNetworkStatusType;

/**
 * @brief LIN transmit/receive context
 *
 * Main structure containing the LIN state machine, buffers, and configuration.
 */
typedef struct {
    /* Current state of the LIN protocol state machine; volatile because modified in ISR */
    volatile LIN_STATE state;

    /* Role of this node on the bus: LIN_COMMANDER or LIN_RESPONDER */
    Lin_NodeType node;

    /* True when the inter-byte timeout counter is active and monitoring for the next byte */
    volatile bool timeoutEnabledForInterByte;

    /* True when an error requires the state machine to be reset on the next falling edge */
    volatile bool errorResetScenario;

    /* True when a rising edge (stop bit) has been detected after a falling edge during data phase */
    volatile bool checkPosEdgeDetected;

    /* True when a falling edge arrived before the expected stop bit rising edge (framing suspect) */
    volatile bool checkStopBitErrorOccured;

    /* Pointer to the message table used for PID-to-frame lookup */
    LIN_table_record_t *msgTbl;
    /* Index of the last matched entry in the message table */
    uint8_t msgTblIdx;

    /* Last byte transmitted, used for loopback bit-error detection */
    uint8_t Tx_LAST_BYTE;

    /* Current channel status reported to the application layer */
    Lin_StatusType status;

    /* PID of the frame currently being processed, retained for checksum calculation */
    uint8_t currentPID;

    /* Pointer to the shared timing/auto-baud context; volatile because modified in ISR */
    volatile Lin_timing_t *timing;

    /* Activity state of the channel: idle or busy */
    Lin_ChannelActivityStatusType channelStatus;

    /* Network-level power state: operational, sleep pending, or sleep */
    Lin_ChannelNetworkStatusType networkStatus;

    /* Most recent error code set by LIN_SetError */
    LIN_ERROR lastError;
    /* Running count of all errors detected since initialization */
    uint32_t errorCount;
    /* Running count of successfully completed frames since initialization */
    uint32_t successCount;
    /* True if a response error flag is pending for the application */
    bool responseError;

    /* True when currently receiving a diagnostic (0x3C/0x3D) frame */
    bool receivingDiagnosticFrame;
    /* Expected number of data bytes for the current frame */
    uint8_t expectedDataLength;

    /* PDU context for the currently received frame */
    Lin_PduType rxPdu;
    /* PDU context for the currently transmitted frame */
    Lin_PduType txPdu;

    /* Selects whether SYNC check applies to first frame only or all frames */
    LIN_SYNC_CHECK_FRAME_TYPE syncCheckType;
    /* Running checksum accumulator word during data byte processing */
    LIN_word_t tempChksum;
    /* Callback function pointer for the current frame, set during PID lookup */
    LIN_function_ptr_t callbackFunction;
    /* True if an external wakeup pulse meeting timing requirements was detected */
    bool extWakeUpStatus;
    
} Lin_TxRxCtx_t;

/* Responder message table array, defined in the application source file */
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
extern uint8_t LIN_calculatePID(uint8_t frameID);

/**
 * @brief Called after successful TX transmission (implemented in lin_responder.c)
 */
extern void LIN_processMessage_Tx(void);

/**
 * @brief Called after successful wake-up (implemented in lin_responder.c)
 */
extern void LIN_processWakeUp(void);

/**
 * @brief Called after successful sleep (implemented in lin_responder.c)
 */
extern void LIN_processSleep(void);

/**
 * @brief Called when an error is detected (implemented in lin_responder.c)
 * @param error The error type that occurred
 */
extern void LIN_processError(LIN_ERROR error);

/* Configure the LIN peripheral and context for responder (slave) node operation */
extern void Lin_ConfigResponder();

/* Configure the LIN peripheral and context for commander (master) node operation */
extern void Lin_ConfigCommander();

/* Initiate transmission of a LIN frame from the commander; returns true on success */
extern uint8_t LIN_SendData(Lin_PduType *txPdu);

/* Update network and channel status to sleep and reconfigure peripheral accordingly */
extern void LIN_TransitionToSleepState(Lin_TxRxCtx_t *lin);

/* Arm the positive-edge interrupt to detect an incoming wakeup pulse */
extern void Lin_EnableWakeupDetection();

/* Restore operational status and reconfigure peripheral after a wakeup event */
extern void Lin_TransitionToWakeUpState();

/* Transmit the LIN go-to-sleep master request frame (0x3C with 0x00 first byte) */
extern void LIN_SendGoToSleepSignal();

/* Transmit a LIN wakeup pulse (dominant pulse of 250 µs–5 ms) */
extern void Lin_SendWakeupSignal();

/* End of header guard */
#endif /* LIN_CONFIG_H_ */