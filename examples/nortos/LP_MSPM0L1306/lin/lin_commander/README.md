## Example Summary

This example demonstrates interrupt-based LIN Commander operation on the
LP-MSPM0L1306 LaunchPad. It showcases PUBLISH frames (transmit), SUBSCRIBE
frames (receive), and RESPONDER_TO_RESPONDER frames with interrupt-driven
transmission and reception. The example also includes comprehensive LIN error
scenario testing such as sync errors, checksum errors, PID parity errors,
communication timeouts, and incomplete responses. Button-driven sequential
operations allow cycling through all LIN frame types and error conditions.

## Peripherals & Pin Assignments

| Peripheral | Pin  | Function                             |
| ---------- | ---- | ------------------------------------ |
| GPIOA      | PA1  | Standard Output                      |
| GPIOA      | PA26 | Standard Output                      |
| GPIOA      | PA27 | Standard Output                      |
| GPIOA      | PA18 | Standard with Wake Input             |
| GPIOA      | PA14 | Standard Input with internal pull-up |
| SYSCTL     |      |                                      |
| UART0      | PA22 | RX Pin                               |
| UART0      | PA8  | TX Pin                               |
| EVENT      |      |                                      |
| DEBUGSS    | PA20 | Debug Clock                          |
| DEBUGSS    | PA19 | Debug Data In Out                    |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0L1306](https://www.ti.com/tool/LP-MSPM0L1306) for LaunchPad information, including user guide and hardware files.

| Pin  | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| ---- | ---------- | -------- | ------------- | ------------------ |
| PA22 | GPIOA      | PA22     | J1_8          | <ul><li>PA22 can be connected to photodiode D1<br><ul><li>`J6 OFF` Disconnect from photodiode D1</ul></ul> |
| PA26 | GPIOA      | PA26     | J4_38         | <ul><li>PA26 can be connected to LED2 Red<br><ul><li>`J12 ON` Connect to LED2 Red<br><li>`J12 OFF` Disconnect from LED2 Red</ul></ul> |
| PA27 | GPIOA      | PA27     | J4_37         | <ul><li>PA27 can be connected to LED2 Blue<br><ul><li>`J13 ON` Connect to LED2 Blue<br><li>`J13 OFF` Disconnect from LED2 Blue</ul></ul> |
| PA18 | GPIOA      | PA18     | J3_26         | <ul><li>PA18 can be connected to S1 button to VCC with external pull-down<br><ul><li>`J11 ON` Connect S1 button and external pull-down<br><li>`J11 OFF` Disconnect S1 button and external pull-down</ul><br><li>PA18 can be connected to XDS-110 BSL_Invoke<br><ul><li>`J101 17:18 OFF` Disconnect pin to XDS-110 BSL Invoke</ul></ul> |
| PA14 | GPIOA      | PA14     | N/A           | <ul><li>PA14 is connected to S2 button to GND with no external pull resistor<br><ul><li>Press `S2` button to connect pin to GND<br><li>Don't use `S2` button if not needed by application</ul></ul> |
| PA22  | UART0      | RX       | J1_3          | <ul><li>PA22 can be used as UART RX connected to XDS-110 backchannel or to boosterpack connector:<br><ul><li>To use backchannel UART on J101:<br>  `J17 1:2`: Connects XDS-110 backchannel UART RX to `J101`<br>  `J101 7:8` Connects XDS-110 backchannel to UART RX<br><li>To use UART on boosterpack connector:<br>  `J17 2:3`: Connects UART RX to `J1_3`</ul></ul> |
| PA8  | UART0      | TX       | J1_4          | <ul><li>PA8 can be used as UART TX connected to XDS-110 backchannel or to boosterpack connector:<br><ul><li>To use backchannel UART on J101:<br>  `J16 1:2`: Connects XDS-110 backchannel UART TX to `J101`<br>  `J101 9:10` Connects XDS-110 backchannel to UART TX<br><li>To use UART on boosterpack connector:<br>  `J16 2:3`: Connects UART TX to` J1_4`</ul></ul> |
| PA20 | DEBUGSS    | SWCLK    | J2_13         | <ul><li>PA20 is used by SWD during debugging<br><ul><li>`J101 15:16 ON` Connect to XDS-110 SWCLK while debugging<br><li>`J101 15:16 OFF` Disconnect from XDS-110 SWCLK if using pin in application</ul></ul> |
| PA19 | DEBUGSS    | SWDIO    | J2_17         | <ul><li>PA19 is used by SWD during debugging<br><ul><li>`J101 13:14 ON` Connect to XDS-110 SWDIO while debugging<br><li>`J101 13:14 OFF` Disconnect from XDS-110 SWDIO if using pin in application</ul></ul> |

### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0L1306 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**â†’**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP-MSPM0L1306 User's Guide](https://www.ti.com/lit/slau869).

## Example Usage

### Hardware Setup
Connect the LP-MSPM0L1306 LaunchPad to a LIN Responder or Network Analyzer via
the compatible BOOSTXL-CANFD-LIN BoosterPack:
- LaunchPad GND (J2_19)          -> BoosterPack GND
- LaunchPad PA1 (LIN_ENABLE)     -> BoosterPack LIN_ENABLE
- LaunchPad UART0 TX (PA8, J1_4)   -> BoosterPack UART TX (via J16)
- LaunchPad UART0 RX (PA22, J1_3)  -> BoosterPack UART RX (via J17)

Configure jumpers for boosterpack connection mode (J16 2:3, J17 2:3).
The BOOSTXL-CANFD-LIN BoosterPack can be stacked directly on the LP-MSPM0L1306
LaunchPad.

### Message Configuration
The LIN Commander message table defines 4 messages using enhanced checksum at 32MHz/19200 baud:
- **Message 0 (ID 0x10)**: PUBLISH - Commander sends 8 bytes to responder(s)
- **Message 1 (ID 0x20)**: SUBSCRIBE - Commander requests 8-byte response from responder
- **Message 2 (ID 0x30)**: RESPONDER_TO_RESPONDER - Header only (8 bytes)
- **Message 3 (ID 0x31)**: RESPONDER_TO_RESPONDER - Header only (8 bytes)

### Operation Flow
Press S1 button (PA18) to cycle through the following interrupt-driven operations sequentially:

1. **OP_STATE_PUBLISH**: Send PUBLISH frame with 8 bytes of data
   - LED1 (PA27 Blue) pulses briefly on transmission complete

2. **OP_STATE_SUBSCRIBE**: Send SUBSCRIBE header and wait for responder response
   - LED2 (PA26 Red) pulses on successful data reception (callback triggered)

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
