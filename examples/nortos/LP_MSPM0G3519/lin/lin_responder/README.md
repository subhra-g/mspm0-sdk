## Example Summary

This example configures the UART as a LIN Responder node that implements a passive
message reception and transmission handler using the LIN 2.0 protocol with enhanced
checksum. The responder:
- Receives data frames (SUBSCRIBE) from the commander and echoes them back with +1 offset
- Transmits data frames (PUBLISH) to the commander
- Supports responder-to-responder communication where the commander sends headers
- Performs automatic baud rate detection and adjustment
- Supports bus sleep/wake-up sequences with low-power operation

LIN is a feature only usable with a UART Extend instance.

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

Visit [LP_MSPM0G3519](https://www.ti.com/tool/LP-MSPM0G3519) for LaunchPad information, including user guide and hardware files.

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
This project was developed for a superset device included in the MSPM0G3519 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP-MSPM0G3519 User's Guide](https://www.ti.com/lit/slau947).

## Example Usage

### Hardware Setup

1. **Connect LaunchPad to LIN BoosterPack:**
   - Responder GND         -> BoosterPack GND
   - Responder LIN_ENABLE  -> BoosterPack LIN_EN
   - Responder TX (PA10)   -> BoosterPack UATX (LIN TX)
   - Responder RX (PA11)   -> BoosterPack UARX (LIN RX)

2. **Connect LIN Bus:**
   - Connect the LIN Commander BoosterPack and the LIN Responder BoosterPack using the LIN bus lines in J5.
   - The two LaunchPads will communicate via the LIN bus.

**Note:** The BOOSTXL-CANFD-LIN BoosterPack is not directly compatible with the LP_MSPM0G3519 LaunchPad since the pins on the UART connector don't support LIN. The BoosterPack should NOT be stacked on top of the LaunchPad.

**Alternative Setup:** A Network Analyzer compatible with LIN 2.0 can be substituted for the LIN Commander. Make the following connections:
   - Network Analyzer GND    -> BoosterPack GND
   - Network Analyzer LINbus -> BoosterPack LIN_TERM

### Configuration

The LIN Responder is configured to run at 32MHz at 19200 baud. These settings can be updated in SysConfig.

**Automatic Baud Rate Synchronization:**
- Enabled by default. Measurements are taken from the SYNC field to determine and adjust the baud rate
- Tolerance: ±5% (per LIN_AUTO_BAUD_MIN/MAX in lin_config.h)
- To disable: In "lin_config.h", set "AUTO_BAUD_ENABLED" to false
- Recalibration triggered after 3 SYNC measurements deviate from nominal

**Low-Power Operation:**
- Responder enters sleep state on startup and waits for bus activity via wakeup detection
- Uses Wait-For-Interrupt (__WFI()) in the main loop for minimal power consumption
- Automatically transitions to sleep/wake states based on LIN bus commands

### Running the Example

1. Compile and load the example on both the LIN Commander and LIN Responder LaunchPads.
2. Run the Responder first (enters sleep waiting for bus activity).
3. Run the Commander (sends frames to wake up the Responder).

### Responder Message Table

The responder handles the following frame types:

| MessageID | PID Range | Frame Type | Direction | Function |
|-----------|-----------|-----------|-----------|----------|
| 0x10 | Auto-calculated | SUBSCRIBE | RX (8 bytes) | Receives data from commander; echoes back with +1 offset to TX buffer |
| 0x20 | Auto-calculated | PUBLISH | TX (8 bytes) | Transmits data frame to commander |
| 0x30 | Auto-calculated | Responder-to-Responder | TX (8 bytes) | Commander sends header only; responder ignores |

**Data Processing:**
- When a SUBSCRIBE (RX) frame is received, the responder copies all 8 bytes to the TX buffer with +1 offset applied to each byte
- When a PUBLISH (TX) frame is transmitted, the first data byte is automatically incremented for the next transmission
- TX buffer is initialized with sequential values: [0x00, 0x01, 0x02, ..., 0x07]

**Customization:**
The message table, PID mappings, and callback handlers can be modified in the "responderMessageTable" array in [lin_responder.c](lin_responder.c).

### Power Management

- **LIN_TransitionToSleepState():** Transitions the responder to sleep mode
- **Lin_EnableWakeupDetection():** Enables positive-edge interrupt detection for wakeup pulses
- **Lin_TransitionToWakeUpState():** Restores operational status after wakeup
- **DL_SYSCTL_enableSleepOnExit():** Automatically enters low-power mode when exiting ISR

### LED Indicators

- **LED1 (PB15):** Brief pulse each time a SUBSCRIBE (RX) frame is received
- **LED2 (PB22/PB26):** Brief pulse each time a PUBLISH (TX) frame is transmitted
- **Both LEDs:** 3 rapid blinks (50ms on/off) when an error is detected (checksum, framing, PID parity, etc.)
