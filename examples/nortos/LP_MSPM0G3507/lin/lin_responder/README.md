## Example Summary

This example demonstrates interrupt-based LIN Responder operation on the
LP_MSPM0G3507 LaunchPad. It showcases SUBSCRIBE frames (receive from commander),
PUBLISH frames (transmit to commander), and RESPONDER_TO_RESPONDER frames using
interrupt-driven transmission and reception. The responder includes automatic baud
rate synchronization that adjusts the baud rate based on the sync field if the
commander transmits at a rate within Â±15% of the expected rate. LED feedback
indicates successful packet reception (LED Blue) and transmission (LED Red), with
error detection triggering LED blinks on both indicators.

## Peripherals & Pin Assignments

| Peripheral | Pin  | Function          |
| ---------- | ---- | ----------------- |
| GPIOB      | PB15 | Standard Output   |
| GPIOB      | PB22 | Standard Output   |
| GPIOB      | PB26 | Standard Output   |
| SYSCTL     |      |                   |
| UART0      | PA11 | RX Pin            |
| UART0      | PA10 | TX Pin            |
| EVENT      |      |                   |
| DEBUGSS    | PA20 | Debug Clock       |
| DEBUGSS    | PA19 | Debug Data In Out |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0G3507](https://www.ti.com/tool/LP-MSPM0G3507) for LaunchPad information, including user guide and hardware files.

| Pin  | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| ---- | ---------- | -------- | ------------- | ------------------ |
| PB15 | GPIOB      | PB15     | J2_17         | N/A                |
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
Connect the LP_MSPM0G3507 LaunchPad to a LIN Commander or Network Analyzer via a
LIN BoosterPack interface:
- LaunchPad GND (J3_1)           -> BoosterPack GND
- LaunchPad PB15 (J2_17)         -> BoosterPack LIN_ENABLE
- LaunchPad UART0 TX (PA10, J26_5)  -> BoosterPack UART TX (via J21/R62)
- LaunchPad UART0 RX (PA11, J26_6)  -> BoosterPack UART RX (via J22/R63)

Configure jumpers for CAN/LIN connector connection mode (J21/R62, J22/R63).
Note: The BOOSTXL-CANFD-LIN BoosterPack is not directly compatible with the
LP_MSPM0G3507 LaunchPad as the UART connector pins do not support LIN mode.
Connect via external jumpers instead of stacking.

### Message Configuration
The LIN Responder message table defines 3 messages using enhanced checksum at 32MHz/19200 baud
with automatic baud rate synchronization enabled by default (Â±15% tolerance):
- **Message 0 (ID 0x10)**: SUBSCRIBE - Responder receives 8 bytes from commander
  - Callback: `LIN_processMessage_Rx()` - echoes received data (incremented by 1) in next transmission
- **Message 1 (ID 0x20)**: PUBLISH - Responder sends 8 bytes to commander
  - Callback: `LIN_processMessage_Tx()` - increments first byte for next transmission
- **Message 2 (ID 0x30)**: RESPONDER_TO_RESPONDER - Responder sends 8 bytes to another responder

### Operation Flow
The responder operates in interrupt-driven low-power mode, responding to commands from the
LIN Commander. The responder handles the following PID commands:

1. **PID 0x10 (SUBSCRIBE)**:
   - Commander sends 8 bytes of data
   - Responder receives data, copies it to TX buffer with each byte incremented by 1
   - LED Blue (PB22) pulses briefly on successful reception

2. **PID 0x20 (PUBLISH)**:
   - Commander requests 8 bytes from responder
   - Responder transmits current data from TX buffer
   - First byte incremented for next transmission
   - LED Red (PB26) pulses briefly on successful transmission

3. **PID 0x30 (RESPONDER_TO_RESPONDER)**:
   - Commander sends header only
   - Responder responds with 8 bytes to another responder on the LIN bus

4. **Error Detection**:
   - Both LEDs blink 3 times to indicate LIN error conditions
   - Errors are detected automatically by the LIN driver

### Features
- **Automatic Baud Rate Synchronization**: Enabled by default. Responder automatically
  adjusts baud rate if commander operates at Â±15% different rate. Disable by setting
  `AUTO_BAUD_ENABLED` to `false` in [lin_config.h](lin_config.h).
- **Low Power Operation**: Responder enters low-power sleep mode when waiting for bus activity.
  Interrupts wake the device to process LIN frames.
- **Interrupt-Driven**: All LIN communication uses interrupt handlers for efficiency.

### Customization
Modify the `responderMessageTable` array in [lin_responder.c](lin_responder.c) to change
message IDs, data sizes, checksum types, or callback functions. Update baud rate settings
in SysConfig if needed.
