## Example Summary

This example demonstrates interrupt-based LIN Responder operation on the
LP_MSPM0C1106 LaunchPad. It showcases SUBSCRIBE frames (receive from commander),
PUBLISH frames (transmit to commander), and RESPONDER_TO_RESPONDER frames using
interrupt-driven transmission and reception. The responder includes automatic baud
rate synchronization that adjusts the baud rate based on the sync field if the
commander transmits at a rate within Â±15% of the expected rate. LED feedback
indicates successful packet reception (LED2 Blue) and transmission (LED2 Red), with
error detection triggering LED blinks on both indicators.

## Peripherals, Pin Functions, MCU Pins, Launchpad Pins
| Peripheral | Function | MCU Pin | Launchpad Pin | Launchpad Settings |
| --- | --- | --- | --- | --- |
| UART0 | RX Pin | PA11 | J4_35 | |
| UART0 | TX Pin | PA10 | J2_15 | |
| EVENT | | | | |
| DMA | | | | |
| CRCP | | | | |
| TIMG0 | | | | |
| BOARD | Debug Clock | PA20 | J14_4 | |
| BOARD | Debug Data In Out | PA19 | J14_4 | |
| GPIOA | Standard Output | PB20 | LED2 Blue | |
| GPIOB | Standard Output | PB24 | LED2 Red | |
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
Connect the LP_MSPM0C1106 LaunchPad to a LIN Commander or Network Analyzer via a
LIN BoosterPack interface:
- LaunchPad GND (J3_1)           -> BoosterPack GND
- LaunchPad PB15 (J3_24)         -> BoosterPack LIN_ENABLE
- LaunchPad UART0 TX (PA10)      -> BoosterPack UART TX
- LaunchPad UART0 RX (PA11)      -> BoosterPack UART RX

Note: The BOOSTXL-CANFD-LIN BoosterPack is not directly compatible with the
LP_MSPM0C1106 LaunchPad as the UART connector pins do not support LIN mode.
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
   - LED2 Blue (PB20) pulses briefly on successful reception

2. **PID 0x20 (PUBLISH)**:
   - Commander requests 8 bytes from responder
   - Responder transmits current data from TX buffer
   - First byte incremented for next transmission
   - LED2 Red (PB24) pulses briefly on successful transmission

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
