## Example Summary

I2S Controller example which uses DMA to output a sine wave onto an audio data
pin. When connected to an audio amplifier board (TAS5805EVM for example), which
has a speaker hooked up, a continuous tone will play from the speaker.

This example can be used to validate I2S communication, as a starting point to
enable I2S functionality, or to create custom drivers.

## Peripherals, Pin Functions, MCU Pins, Launchpad Pins
| Peripheral | Function | MCU Pin | Launchpad Pin | Launchpad Settings |
| --- | --- | --- | --- | --- |
| SYSCTL |  |  |  |  |
| I2S0 | Data Pin 0 | PA13 | J3_29 |  |
| I2S0 | Data Pin 1 | PB13 |  |  |
| I2S0 | BCLK Pin | PA9 | J1_9 |  |
| I2S0 | WCLK Pin | PB10 | J2_37 |  |
| BOARD | Debug Clock | PA20 | J101_16 |  |
| BOARD | Debug Data In Out | PA19 | J101_14 |  |

### Device Migration Recommendations
This project was developed for a superset device included in the MSPM0 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the LP-MSPM0G5187 User's Guide.

## Example Usage
Connect the MPSM0 to an audio amplifier board, following the specific instructions for your board.

Run the example. It will play a continuous tone via I2S from the speaker.
