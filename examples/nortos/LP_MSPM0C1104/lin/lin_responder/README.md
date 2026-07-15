## Example Summary

This example demonstrates interrupt-based LIN Responder operation on the
LP_MSPM0C1104 LaunchPad. It showcases SUBSCRIBE frames (receive from commander),
PUBLISH frames (transmit to commander), and RESPONDER_TO_RESPONDER frames using
interrupt-driven transmission and reception. The responder includes automatic baud
rate synchronization that adjusts the baud rate based on the sync field if the
commander transmits at a rate within Â±15% of the expected rate. LED feedback
indicates successful packet reception and transmission, with error detection
triggering multiple LED blinks.

**Note**: Example requires Rev E3 or later of the MSPM0C1104 LaunchPad.

## Peripherals & Pin Assignments

| Peripheral | Pin  | Function          |
| ---------- | ---- | ----------------- |
| GPIOA      | PA4  | Standard Output   |
| GPIOA      | PA17 | Standard Output   |
| SYSCTL     |      |                   |
| UART0      | PA26 | RX Pin            |
| UART0      | PA27 | TX Pin            |
| EVENT      |      |                   |
| DEBUGSS    | PA20 | Debug Clock       |
| DEBUGSS    | PA19 | Debug Data In Out |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0C1104](https://www.ti.com/tool/LP-MSPM0C1104) for LaunchPad information, including user guide and hardware files.

| Pin  | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| ---- | ---------- | -------- | ------------- | --- |
| PA4  | GPIOA      | PA4      | J2_14         | N/A |
| PA17 | GPIOA      | PA17     | J2_18         | N/A |
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

SysConfig allows developers to easily configure unused pins by selecting **Board**â†’**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP-MSPM0C1104 User's Guide](https://www.ti.com/lit/slau908).

## Example Usage

### Hardware Setup
Connect the LP_MSPM0C1104 LaunchPad to a LIN Commander or Network Analyzer via a
LIN BoosterPack interface:
- LaunchPad GND (J2_19)          -> BoosterPack GND
- LaunchPad PA4 (J2_14)          -> BoosterPack LIN_ENABLE
- LaunchPad UART0 TX (PA27)      -> BoosterPack UART TX
- LaunchPad UART0 RX (PA26)      -> BoosterPack UART RX

Note: The BOOSTXL-CANFD-LIN BoosterPack is not directly compatible with the
LP_MSPM0C1104 LaunchPad as the UART connector pins do not support LIN mode.
Connect via external jumpers instead of stacking.

Configure jumpers to disconnect XDS-110 backchannel UART (J101 5:6 OFF, 7:8 OFF)
if using the boosterpack connector.

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
   - LED1 pulses briefly on successful reception

2. **PID 0x20 (PUBLISH)**:
   - Commander requests 8 bytes from responder
   - Responder transmits current data from TX buffer
   - First byte incremented for next transmission
   - LED1 pulses briefly on successful transmission

3. **PID 0x30 (RESPONDER_TO_RESPONDER)**:
   - Commander sends header only
   - Responder responds with 8 bytes to another responder on the LIN bus

4. **Error Detection**:
   - LED1 blinks 3 times to indicate LIN error conditions
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
