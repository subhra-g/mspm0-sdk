## Example Summary

This example demonstrates interrupt-based LIN Commander operation on the
LP_MSPM0G3507 LaunchPad. It showcases PUBLISH frames (transmit), SUBSCRIBE
frames (receive), and RESPONDER_TO_RESPONDER frames with interrupt-driven
transmission and reception. The example also includes comprehensive LIN error
scenario testing such as sync errors, checksum errors, PID parity errors,
communication timeouts, and incomplete responses. Button-driven sequential
operations allow cycling through all LIN frame types and error conditions.

## Peripherals & Pin Assignments

| Peripheral | Pin  | Function                             |
| ---------- | ---- | ------------------------------------ |
| GPIOA      | PA18 | Standard with Wake Input             |
| GPIOB      | PB15 | Standard Output                      |
| GPIOB      | PB21 | Standard Input with internal pull-up |
| GPIOB      | PB22 | Standard Output                      |
| GPIOB      | PB26 | Standard Output                      |
| SYSCTL     |      |                                      |
| UART0      | PA11 | RX Pin                               |
| UART0      | PA10 | TX Pin                               |
| EVENT      |      |                                      |
| DEBUGSS    | PA20 | Debug Clock                          |
| DEBUGSS    | PA19 | Debug Data In Out                    |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0G3507](https://www.ti.com/tool/LP-MSPM0G3507) for LaunchPad information, including user guide and hardware files.

| Pin  | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| ---- | ---------- | -------- | ------------- | ------------------ |
| PA18 | GPIOA      | PA18     | J3_26/J3_29   | <ul><li>PA18 can be connected to S1 button to VCC with external pull-down<br><ul><li>`J8 ON` Connect S1 button and external pull-down<br><li>`J8 OFF` Disconnect S1 button and external pull-down</ul><br><li>PA18 can be connected to XDS-110 BSL_Invoke<br><ul><li>`J101 17:18 OFF` Disconnect pin to XDS-110 BSL Invoke</ul><br><li>PA18 can be connected to boosterpack pin J3_29<br><ul><li>`J15 2:3` Connect pin to J3_29<br><li>`J15 1:2/OFF` Disconnect pin from J3_29</ul><br><li>PA18 can be connected to boosterpack pin J3_26<br><ul><li>`R58` is soldered and connects pin to J3_26 by default</ul></ul> |
| PB15 | GPIOB      | PB15     | J2_17         | N/A |
| PB21 | GPIOB      | PB21     | J27_4         | <ul><li>PB21 is connected to S2 button to GND with no external pull resistor<br><ul><li>Press `S2` button to connect pin to GND<br><li>Don't use `S2` button if not needed by application</ul></ul> |
| PB22 | GPIOB      | PB22     | J27_5         | <ul><li>PB22 can be connected to LED2 Blue<br><ul><li>`J5 ON` Connect to LED2 Blue<br><li>`J15 OFF` Disconnect from LED2 Blue</ul></ul> |
| PB26 | GPIOB      | PB26     | J27_8         | <ul><li>PB26 can be connected to LED2 Red<br><ul><li>`J6 ON` Connect to LED2 Red<br><li>`J6 OFF` Disconnect from LED2 Red</ul></ul> |
| PA11 | UART0      | RX       | J4_33/J26_6   | <ul><li>PA11 can be used as UART RX connected to XDS-110 backchannel, to boosterpack connector or to CAN/LIN connector:<br><ul><li>To use backchannel UART on J101:<br>  `J22 1:2`: Connects XDS-110 backchannel UART RX to `J101`<br>  `J101 7:8` Connects XDS-110 backchannel to UART RX<br><li>To use UART on boosterpack connector:<br>  `J22 2:3`: Connects UART RX to `J4_33`<br><li>To use on J26 CAN/LIN connector:<br>  `R63` is populated by default and connects pin to `J26_6`</ul></ul> |
| PA10 | UART0      | TX       | J4_34/J26_5   | <ul><li>PA10 can be used as UART TX connected to XDS-110 backchannel, to boosterpack connector or to CAN/LIN connector:<br><ul><li>To use backchannel UART on J101:<br>  `J21 1:2`: Connects XDS-110 backchannel UART TX to `J101`<br>  `J101 9:10` Connects XDS-110 backchannel to UART TX<br><li>To use UART on boosterpack connector:<br>  `J21 2:3`: Connects UART TX to `J4_34`<br><li>To use on J26 CAN/LIN connector:<br>  `R62` is populated by default and connects pin to `J26_5`</ul></ul> |
| PA20 | DEBUGSS    | SWCLK    | N/A           | <ul><li>PA20 is used by SWD during debugging<br><ul><li>`J101 15:16 ON` Connect to XDS-110 SWCLK while debugging<br><li>`J101 15:16 OFF` Disconnect from XDS-110 SWCLK if using pin in application</ul></ul> |
| PA19 | DEBUGSS    | SWDIO    | N/A           | <ul><li>PA19 is used by SWD during debugging<br><ul><li>`J101 13:14 ON` Connect to XDS-110 SWDIO while debugging<br><li>`J101 13:14 OFF` Disconnect from XDS-110 SWDIO if using pin in application</ul></ul> |

### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0G3507 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**â†’**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP-MSPM0G3507 User's Guide](https://www.ti.com/lit/slau873).

## Example Usage

### Hardware Setup
Connect the LP_MSPM0G3507 LaunchPad to a LIN Responder or Network Analyzer via
jumper wires (the BOOSTXL-CANFD-LIN BoosterPack is not directly compatible):
- LaunchPad GND                 -> BoosterPack GND
- LaunchPad PB15 (J2_17)        -> BoosterPack LIN_ENABLE
- LaunchPad UART0 TX (PA10, J26_5) -> BoosterPack UART TX (via J21/R62)
- LaunchPad UART0 RX (PA11, J26_6) -> BoosterPack UART RX (via J22/R63)

Configure jumpers as indicated in the Peripherals & Pin Assignments table for
the desired connection method (J26 CAN/LIN connector, boosterpack connector, or
XDS-110 backchannel).

### Message Configuration
The LIN Commander message table defines 4 messages using enhanced checksum at 32MHz/19200 baud:
- **Message 0 (ID 0x10)**: PUBLISH - Commander sends 8 bytes to responder(s)
- **Message 1 (ID 0x20)**: SUBSCRIBE - Commander requests 8-byte response from responder
- **Message 2 (ID 0x30)**: RESPONDER_TO_RESPONDER - Header only (8 bytes)
- **Message 3 (ID 0x31)**: RESPONDER_TO_RESPONDER - Header only (8 bytes)

### Operation Flow
Press S1 button (PA18) to cycle through the following interrupt-driven operations sequentially:

1. **OP_STATE_PUBLISH**: Send PUBLISH frame with 8 bytes of data
   - LED1 (PB22) pulses briefly on transmission complete

2. **OP_STATE_SUBSCRIBE**: Send SUBSCRIBE header and wait for responder response
   - LED2 (PB26) pulses on successful data reception (callback triggered)

3. **OP_STATE_RESP_TO_RESP_1**: Send responder-to-responder header 1
   - LED1 pulses briefly

4. **OP_STATE_RESP_TO_RESP_2**: Send responder-to-responder header 2
   - LED1 pulses briefly

5. **OP_STATE_SLEEP**: Transmit LIN sleep command
   - Both LEDs light up briefly

6. **OP_STATE_WAKEUP**: Transmit LIN wakeup signal
   - Both LEDs light up briefly

7. **OP_STATE_SYNC_ERROR**: Send frame with invalid sync byte (0xAA instead of 0x55)

8. **OP_STATE_CHKSUM_ERROR**: Send frame with incorrect enhanced checksum

9. **OP_STATE_PID_PARITY_ERROR**: Send PID with inverted parity bits

10. **OP_STATE_COM_NO_RES_ERROR**: Send unknown PID and wait for timeout using timer-based RX

11. **OP_STATE_COM_INCMPLT_RES_ERROR**: Request oversized response (9 bytes instead of 8)

12. **OP_STATE_RES_NO_RES_ERROR**: Send PUBLISH with zero data bytes

13. **OP_STATE_RES_INCMPLT_RES_ERROR**: Send PUBLISH with only 5 data bytes

Error conditions trigger both LEDs to blink 3 times (error handler).
The cycle repeats after the last operation.

### Customization
Modify the `commanderMessageTable` array in [lin_commander.c](lin_commander.c) to change
message IDs, data sizes, checksum types, or frame directions.
