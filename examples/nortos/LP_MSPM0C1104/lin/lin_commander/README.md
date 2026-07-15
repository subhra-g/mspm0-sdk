## Example Summary

This example configures the UART as a LIN Commander and demonstrates LIN 2.0 protocol operations including transmit, receive, sleep/wakeup, and error handling using enhanced checksums. The application implements a state machine that cycles through various operational modes triggered by button presses, including normal publish/subscribe frames, responder-to-responder frames, and error condition generation for testing error detection and recovery.
LIN is a feature only usable with a UART Extend instance.
This example is provided for reference purposes only.

**Note**: Example requires Rev E3 or later of the MSPM0C1104 LaunchPad.

## Peripherals & Pin Assignments

| Peripheral | Pin  | Function                             |
| ---------- | ---- | ------------------------------------ |
| GPIOA      | PA4  | Standard Output                      |
| GPIOA      | PA17 | Standard Output                      |
| GPIOA      | PA16 | Standard Input with internal pull-up |
| GPIOA      | PA6  | Standard Input with internal pull-up |
| SYSCTL     |      |                                      |
| UART0      | PA26 | RX Pin                               |
| UART0      | PA27 | TX Pin                               |
| EVENT      |      |                                      |
| DEBUGSS    | PA20 | Debug Clock                          |
| DEBUGSS    | PA19 | Debug Data In Out                    |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0C1104](https://www.ti.com/tool/LP-MSPM0C1104) for LaunchPad information, including user guide and hardware files.

| Pin  | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| ---- | ---------- | -------- | ------------- | ------------------ |
| PA4  | GPIOA      | PA4      | J2_14         | N/A                |
| PA17 | GPIOA      | PA17     | J2_18         | N/A                |
| PA16 | GPIOA      | PA16     | J2_19         | <ul><li>PA16 is connected to S2 button to GND with no external pull resistor<br><ul><li>Press `S2` button to connect pin to GND<br><li>Don't use `S2` button if not needed by application</ul></ul> |
| PA6  | GPIOA      | PA6      | J1_7          | N/A                |
| PA26 | UART0      | RX       | J1_3          | <ul><li>PA26 can be used as UART RX connected to XDS-110 backchannel or to boosterpack connector:<br><ul><li>To use backchannel UART on J101:<br>  `J101 5:6 ON` Connects XDS-110 backchannel to UART RX<br><li>To use UART on boosterpack connector:<br>  `J101 5:6 OFF`: Disconnects XDS-110 backchannel</ul></ul> |
| PA27 | UART0      | TX       | J1_4          | <ul><li>PA27 can be used as UART TX connected to XDS-110 backchannel or to boosterpack connector:<br><ul><li>To use backchannel UART on J101:<br>  `J101 7:8 ON` Connects XDS-110 backchannel to UART TX<br><li>To use UART on boosterpack connector:<br>  `J101 7:8 OFF`: Disconnects XDS-110 backchannel</ul></ul> |
| PA20 | DEBUGSS    | SWCLK    | J2_11         | <ul><li>PA20 is used by SWD during debugging<br><ul><li>`J101 13:14 ON` Connect to XDS-110 SWCLK while debugging<br><li>`J101 13:14 OFF` Disconnect from XDS-110 SWCLK if using pin in application</ul></ul> |
| PA19 | DEBUGSS    | SWDIO    | J2_17         | <ul><li>PA19 is used by SWD during debugging<br><ul><li>`J101 11:12 ON` Connect to XDS-110 SWDIO while debugging<br><li>`J101 11:12 OFF` Disconnect from XDS-110 SWDIO if using pin in application</ul></ul> |

### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0C1104 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP-MSPM0C1104 User's Guide](https://www.ti.com/lit/slau908).

## Example Usage

Connect the LIN Commander to a LIN BoosterPack with the following connections:
- Commander GND         -> BoosterPack GND
- Commander LIN_ENABLE  -> BoosterPack LIN_EN
- Commander TX          -> BoosterPack UATX (LIN TX)
- Commander RX          -> BoosterPack UARX (LIN RX)

Connect a LIN Responder to a LIN BoosterPack with the following connections:
- Responder GND         -> BoosterPack GND
- Responder LIN_ENABLE  -> BoosterPack LIN_EN
- Responder TX          -> BoosterPack UATX (LIN TX)
- Responder RX          -> BoosterPack UARX (LIN RX)

Note: the BOOSTXL-CANFD-LIN BoosterPack is not directly compatible with the LP_MSPM0C1104 LaunchPad since the pins on the UART connector don't support LIN. For this reason, the BoosterPack shouldn't be stacked on top of the LaunchPad.

Connect the LIN Commander BoosterPack and the LIN Responder BoosterPack using the LIN bus lines in J5.

NOTE: Alternatively, a Network Analyzer compatible with LIN 2.0 can be substituted for a LIN
Responder. To use the Network Analyzer, make the following connections between
the Network Analyzer and LIN BoosterPack:
- Network Analyzer GND    -> BoosterPack GND
- Network Analyzer LINbus -> BoosterPack LIN_TERM

The LIN Commander is configured to run at 24MHz at 19200 baud. These settings can be modified in the application.

Compile, load and run the example.

### Operation Modes

Press the S2 button to cycle through the following LIN operations:

1. **OP_STATE_PUBLISH** - Sends an 8-byte frame (Frame ID 0x10) to responder
2. **OP_STATE_SUBSCRIBE** - Requests 8-byte response from responder (Frame ID 0x20)
3. **OP_STATE_RESP_TO_RESP_1** - Sends responder-to-responder header (Frame ID 0x30)
4. **OP_STATE_RESP_TO_RESP_2** - Sends responder-to-responder header (Frame ID 0x31)
5. **OP_STATE_SLEEP** - Transmits LIN sleep command to put bus to sleep
6. **OP_STATE_WAKEUP** - Sends wake-up pulse to wake the bus
7. **OP_STATE_SYNC_ERROR** - Tests sync byte error detection (sends 0xAA instead of 0x55)
8. **OP_STATE_CHKSUM_ERROR** - Tests checksum error detection (sends wrong checksum)
9. **OP_STATE_PID_PARITY_ERROR** - Tests PID parity error detection (flips parity bits)
10. **OP_STATE_COM_NO_RES_ERROR** - Tests no response error detection (unknown PID)
11. **OP_STATE_COM_INCMPLT_RES_ERROR** - Tests incomplete response error (expects 9 bytes, gets 8)
12. **OP_STATE_RES_NO_RES_ERROR** - Tests missing responder data error (zero length frame)
13. **OP_STATE_RES_INCMPLT_RES_ERROR** - Tests incomplete responder data (expects 8, sends 5)

### LED Indicators

- LED1 pulses (50ms) when a PUBLISH frame or responder-to-responder header is transmitted
- LED1 pulses when data is successfully received on a SUBSCRIBE frame
- Both LEDs blink 3 times (50ms each) when any LIN error is detected
- LED1 stays on for 500ms during sleep state
- LED1 pulses (100ms) during wake-up state

### Message Table Configuration

The message table (`commanderMessageTable`) in lin_commander.c defines the 4 frames:
- Frame 0: PUBLISH (0x10) - 8 bytes, enhanced checksum, commander sends
- Frame 1: SUBSCRIBE (0x20) - 8 bytes, enhanced checksum, commander receives
- Frame 2: RESPONDER_TO_RESPONDER_1 (0x30) - 8 bytes, enhanced checksum, header only
- Frame 3: RESPONDER_TO_RESPONDER_2 (0x31) - 8 bytes, enhanced checksum, header only

The message table, message sizes, frame IDs, and callback functions can be modified in the `commanderMessageTable` array in lin_commander.c.
