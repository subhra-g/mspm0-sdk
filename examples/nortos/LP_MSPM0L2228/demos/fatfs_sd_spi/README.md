## Example Summary

Example project demonstrating usage of the MSPM0 with an SD card through the
FatFS module with communication over SPI.

## Peripherals, Pin Functions, MCU Pins, Launchpad Pins
| Peripheral | Function | MCU Pin | Launchpad Pin | Launchpad Settings |
| --- | --- | --- | --- | --- |
| GPIOA | Standard Output with internal pull-up | PA9 |  |  |
| SYSCTL |  |  |  |  |
| TIMA0 |  |  |  |  |
| SPI0 | SPI SCLK (Clock) | PA11 |  |  |
| SPI0 | SPI PICO (Peripheral In, Controller Out) | PA14 |  |  |
| SPI0 | SPI POCI (Peripheral Out, Controller In) | PA10 |  |  |
| EVENT |  |  |  |  |
| SYSTICK |  |  |  |  |
| BOARD | Debug Clock | PA20 | J101_16 |  |
| BOARD | Debug Data In Out | PA19 | J101_14 |  |

### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0L2228 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#non-sysconfig-compatible-project-migration)
for information about migrating to other MSPM0 devices.

## Example Usage

Connect a compatible SD card via SPI to the MSPM0.
The application will open a file "test.txt", write text to this, and then save the file to the SD card.
