## Example Summary

This example demonstrates a comprehensive LIN Commander implementation on the
LP_MSPM0C1106 LaunchPad. It showcases multiple LIN operations including PUBLISH
(commander transmits to responders), SUBSCRIBE (commander receives from
responders), and RESPONDER_TO_RESPONDER frames. The example also demonstrates
LIN error conditions such as sync errors, checksum errors, PID parity errors,
communication timeouts, and incomplete responses. Sequential button-driven
operations cycle through these various LIN frame types and error scenarios.

## Peripherals, Pin Functions, MCU Pins, Launchpad Pins
| Peripheral | Function | MCU Pin | Launchpad Pin | Launchpad Settings |
| --- | --- | --- | --- | --- |
| UART0 | RX Pin | PA11 | J4_35 | |
| UART0 | TX Pin | PA10 | J2_15 | |
| EVENT | | | | |
| DMA | | | | |
| CRCP | | | | |
| TIMA0 | | | | |
| BOARD | Debug Clock | PA20 | J14_4 | |
| BOARD | Debug Data In Out | PA19 | J14_4 | |
| GPIOA | Standard Input | PA18 | S1 | |
| GPIOA | Standard Output | PB20 | LED2 Blue | |
| GPIOB | Standard Output | PB24 | LED2 Red | |
| GPIOB | Standard Input with pull-up | PA14 | S2 | |
| GPIOB | Standard Output  | PB15 | J3_24 | |

### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0C1106 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**â†’**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP_MSPM0C1106 User's Guide](https://www.ti.com/tool/LP-MSPM0C1106).

## Example Usage

### Hardware Setup
Connect the LP_MSPM0C1106 LaunchPad to a LIN Responder or Network Analyzer via a
LIN BoosterPack interface:
- LaunchPad GND (J3_1)          -> BoosterPack GND
- LaunchPad PB15 (J3_24)        -> BoosterPack LIN_ENABLE
- LaunchPad UART0 TX (PA10)     -> BoosterPack UART TX
- LaunchPad UART0 RX (PA11)     -> BoosterPack UART RX

Note: The BOOSTXL-CANFD-LIN BoosterPack is not directly compatible with the
LP_MSPM0C1106 LaunchPad as the UART connector pins do not support LIN mode.
Connect via external jumpers instead of stacking.

### Message Configuration
The LIN Commander message table defines 4 messages:
- **Message 0 (ID 0x10)**: PUBLISH - Commander sends 8 bytes to responder(s)
- **Message 1 (ID 0x20)**: SUBSCRIBE - Commander requests 8-byte response from responder
- **Message 2 (ID 0x30)**: RESPONDER_TO_RESPONDER - Header only (8 bytes)
- **Message 3 (ID 0x31)**: RESPONDER_TO_RESPONDER - Header only (8 bytes)

All messages use enhanced checksum and operate at 32MHz/19200 baud.

### Operation Flow
Press S1 button to cycle through the following operations sequentially:

1. **OP_STATE_PUBLISH**: Transmit PUBLISH frame
   - LED1 pulses briefly to indicate transmission

2. **OP_STATE_SUBSCRIBE**: Send SUBSCRIBE header and wait for responder data
   - LED2 pulses on successful reception (from callback)

3. **OP_STATE_RESP_TO_RESP_1**: Send responder-to-responder header 1
   - LED1 pulses briefly

4. **OP_STATE_RESP_TO_RESP_2**: Send responder-to-responder header 2
   - LED1 pulses briefly

5. **OP_STATE_SLEEP**: Transmit LIN sleep command
   - Both LEDs briefly light up

6. **OP_STATE_WAKEUP**: Transmit LIN wakeup signal
   - Both LEDs briefly light up

7. **OP_STATE_SYNC_ERROR**: Send frame with invalid sync byte (0xAA instead of 0x55)

8. **OP_STATE_CHKSUM_ERROR**: Send frame with incorrect enhanced checksum

9. **OP_STATE_PID_PARITY_ERROR**: Send PID with invalid parity bits

10. **OP_STATE_COM_NO_RES_ERROR**: Send unknown PID and wait for timeout

11. **OP_STATE_COM_INCMPLT_RES_ERROR**: Request oversized response (9 bytes)

12. **OP_STATE_RES_NO_RES_ERROR**: Send PUBLISH with zero data bytes

13. **OP_STATE_RES_INCMPLT_RES_ERROR**: Send PUBLISH with only 5 data bytes

Error conditions trigger both LEDs to blink 3 times (error handler).
The cycle repeats after the last operation.

### Customization
Modify the `commanderMessageTable` array in [lin_commander.c](lin_commander.c) to change
message IDs, data sizes, checksum types, or frame directions.
