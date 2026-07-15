## Example Summary

This example configures the UART as a LIN Commander node that implements multi-frame
LIN 2.0 communication including:
- Publishing (transmitting) data to responder nodes
- Subscribing (receiving) data from responder nodes
- Responder-to-responder communication (sending headers only)
- Bus sleep/wake-up signaling
- Error injection and handling (checksum errors, incomplete responses)
- State machine-based sequential frame transmission triggered by button press

LIN is a feature only usable with a UART Extend instance.

## Peripherals & Pin Assignments

| Peripheral | Pin  | Function                             |
| ---------- | ---- | ------------------------------------ |
| GPIOA      | PA18 | Standard with Wake Input             |
| GPIOB      | PB15 | Standard Output                      |
| GPIOB      | PB3  | Standard Input with internal pull-up |
| GPIOB      | PB22 | Standard Output                      |
| GPIOB      | PB26 | Standard Output                      |
| SYSCTL     |      |                                      |
| UART0      | PA11 | RX Pin                               |
| UART0      | PA10 | TX Pin                               |
| EVENT      |      |                                      |
| DEBUGSS    | PA20 | Debug Clock                          |
| DEBUGSS    | PA19 | Debug Data In Out                    |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0G3519](https://www.ti.com/tool/LP-MSPM0G3519) for LaunchPad information, including user guide and hardware files.

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
   - Commander GND         -> BoosterPack GND
   - Commander LIN_ENABLE  -> BoosterPack LIN_EN
   - Commander TX (PA10)   -> BoosterPack UATX (LIN TX)
   - Commander RX (PA11)   -> BoosterPack UARX (LIN RX)

2. **Connect LIN Bus:**
   - Connect the LIN Commander BoosterPack and the LIN Responder BoosterPack using the LIN bus lines in J5.
   - The two LaunchPads will communicate via the LIN bus.

**Note:** The BOOSTXL-CANFD-LIN BoosterPack is not directly compatible with the LP_MSPM0G3519 LaunchPad since the pins on the UART connector don't support LIN. The BoosterPack should NOT be stacked on top of the LaunchPad.

**Alternative Setup:** A Network Analyzer compatible with LIN 2.0 can be substituted for the LIN Responder. Make the following connections:
   - Network Analyzer GND    -> BoosterPack GND
   - Network Analyzer LINbus -> BoosterPack LIN_TERM

### Configuration

The LIN Commander is configured to run at 32MHz at 19200 baud. These settings can be modified in the application.

### Running the Example

1. Compile and load the example on both the LIN Commander and LIN Responder LaunchPads.
2. Run the Responder first (enters sleep waiting for bus activity).
3. Run the Commander (will wake up the Responder and begin sequential frame transmission).
4. Press S1 button (PA18 with internal pull-up) to advance through operation states.

### Operation State Machine

The commander cycles through the following operation states each time the S1 button is pressed:

| State | Operation | Description |
|-------|-----------|-------------|
| OP_STATE_PUBLISH | Send Data | Transmits 8 bytes to responder (MessageID 0x10); LED1 pulses on success |
| OP_STATE_SUBSCRIBE | Receive Data | Requests 8 bytes from responder (MessageID 0x20); LED2 pulses on successful RX |
| OP_STATE_RESP_TO_RESP_1 | R2R Frame 1 | Sends header only for MessageID 0x30 (responder-to-responder); LED1 pulses |
| OP_STATE_RESP_TO_RESP_2 | R2R Frame 2 | Sends header only for MessageID 0x31 (responder-to-responder); LED1 pulses |
| OP_STATE_SLEEP | Go to Sleep | Sends LIN sleep command (0x3C with 0x00 first byte); both LEDs on for 500ms |
| OP_STATE_WAKEUP | Send Wakeup | Transmits wakeup pulse (250µs–5ms dominant pulse); both LEDs on for 100ms |
| OP_STATE_CHKSUM_ERROR | Checksum Error | Sends a frame with incorrect checksum for testing error handling |
| OP_STATE_RES_INCMPLT_RES_ERROR | Incomplete Response | Sends 5-byte frame instead of 8 bytes to test incomplete response error |

After reaching OP_STATE_RES_INCMPLT_RES_ERROR, the cycle repeats back to OP_STATE_PUBLISH.

### Commander Message Table

| MessageID | PID Range | Frame Type | Direction | Size | Function |
|-----------|-----------|-----------|-----------|------|----------|
| 0x10 | Auto-calculated | PUBLISH | TX (8 bytes) | 8 | Sends 8 bytes of data to responder |
| 0x20 | Auto-calculated | SUBSCRIBE | RX (8 bytes) | 8 | Receives 8 bytes of data from responder |
| 0x30 | Auto-calculated | R2R | IGNORE | 8 | Responder-to-responder: commander sends header only |
| 0x31 | Auto-calculated | R2R | IGNORE | 8 | Responder-to-responder: commander sends header only |

**TX Data Buffer:**
- Initialized with sequential values: [0x00, 0x01, 0x02, ..., 0x07]
- First byte increments after each successful PUBLISH transmission

**RX Data Buffer:**
- Stored in gLIN.rxPdu.shadowBuffer (max 8 bytes)
- Overwritten each time a SUBSCRIBE frame is successfully received

**Customization:**
The message table, PID mappings, and operation state logic can be modified in [lin_commander.c](lin_commander.c).

### Button Interaction

**S1 Button (PA18):**
- Press to advance to the next operation state
- Each press triggers the corresponding LIN frame transmission/reception
- Button uses internal pull-up; connect to GND when pressed
- GPIO_SWITCHES1_USER_SWITCH_1 interrupt handler sets gProcessCmd flag

### Sleep/Wakeup Sequence

**Sleep Sequence:**
- Commander sends LIN sleep frame (PID 0x3C with data [0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF])
- Both LEDs turn on for 500ms to indicate sleep command sent
- Responder automatically enters sleep state
- Channel transitions to LIN_CHANNEL_SLEEP

**Wakeup Sequence:**
- Commander sends a dominant pulse (250µs minimum per LIN spec, up to 5ms)
- Both LEDs turn on for 100ms to indicate wakeup pulse sent
- Responder wakes from sleep and returns to operational state
- Channel transitions to LIN_CHANNEL_OPERATIONAL

### Error Testing

**Checksum Error (OP_STATE_CHKSUM_ERROR):**
- Manually constructs LIN frame with incorrect checksum (0x00 instead of calculated value)
- Tests responder error handling and recovery

**Incomplete Response Error (OP_STATE_RES_INCMPLT_RES_ERROR):**
- Temporarily reduces message size from 8 bytes to 5 bytes
- Responder sends only 5 bytes instead of expected 8, triggering incomplete response error
- Message size automatically restored to 8 bytes after frame transmission

### LED Indicators

- **LED1 (PB15):** Brief pulse (50ms) on successful PUBLISH or R2R frame transmission
- **LED2 (PB22/PB26):** Brief pulse (50ms) on successful SUBSCRIBE frame reception
- **Both LEDs:** 500ms on during sleep command; 100ms on during wakeup pulse
- **Both LEDs:** 3 rapid blinks (50ms on/off) when an error is detected
