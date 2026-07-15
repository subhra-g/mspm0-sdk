## Example Summary

Secondary Bootloader implements a USB Device Firmware Upgrade (DFU) interface
using the TinyUSB stack. It runs in Main Flash, overriding the ROM BSL, and
allows firmware download and readback over a standard USB DFU connection from
a host PC.

Supported operations:
- Firmware download (DFU_DNLOAD) — erase and program flash sectors
- Firmware upload (DFU_UPLOAD) — read back programmed flash **(Refer to note for careabouts and limitations)**

The USB DFU class is handled by TinyUSB. The bootloader listens for DFU
transfers and programs the MSPM0G5187 internal flash directly.

This example registers itself as the secondary BSL, replacing the primary ROM BSL.
Once loaded, the ROM BSL is no longer accessible. To restore the ROM BSL,
use the SWD_Factory_Reset command.

For more details refer to BSL User Guide.

This example can be used to create a custom Bootloader.

NOTE:
1. The flash region for firmware storage is defined by DFU_FLASH_START_ADDR
and DFU_FLASH_END_ADDR in dfu_app.h. These must be adjusted to match the
target application memory map.
2. While creating a Custom Bootloader, make sure the flash region containing
the custom Bootloader is Static write protected in the BCR configuration.
Otherwise the device may become locked during a bootloading operation.
3. The upload (read-back) region is separately limited by DFU_FLASH_READ_END_ADDR
in dfu_app.h. Limitation: The upload read-back length must be non-64-byte-aligned.

## Peripherals & Pin Assignments

| Peripheral | Pin | Function |
| --- | --- | --- |
| SYSCTL |  |  |
| USBFS0 |  | USB Full-Speed Device (DFU) |
| UC0 | PA0 | UART TX (debug logging) |
| UC0 | PA1 | UART RX (debug logging) |
| GPIOB | PB13 | LED (TinyUSB board indicator) |
| GPIOB | PB14 | Button (TinyUSB board input) |
| DEBUGSS | PA20 | Debug Clock |
| DEBUGSS | PA19 | Debug Data In Out |



### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0G5187 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#manual-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the LP-MSPM0G5187 User's Guide.

## Example Usage

### Device Identification

This example uses TI's Vendor ID (0x2047) and the BSL Product ID (0x0210) for demonstration purposes.
For production use, you MUST update the `idVendor` and `idProduct` fields in `usb_descriptors.c` to match your
device's officially assigned USB Vendor ID and Product ID to avoid conflicts with other USB devices.

### Driver Installation

**Windows:**
Install the WinUSB or libusbK driver using [Zadig](https://zadig.akeo.ie/):
1. Connect the device and invoke BSL so it enumerates as a USB DFU device.
2. Open Zadig, select the DFU device from the dropdown, choose WinUSB (or libusbK), and click "Install Driver".
3. Install [dfu-util](https://dfu-util.sourceforge.net/) and ensure it is on the system PATH.

**Linux:**
No additional driver installation is required. Install dfu-util via the package manager:
```
sudo apt install dfu-util
```
If permission is denied when running dfu-util, either run it with `sudo` or add a udev rule for the device VID/PID.

---

### Steps to Follow 
1. Connect the LP-MSPM0G5187 USB 2.0 connector to the host PC.

2. Compile and load the example.

3. Create BSL invocation condition using BSL Invoke pin or any other invocation method.

4. Once the device enumerates as a USB DFU device on the host, use dfu-util to download or read back firmware. This example has been tested with dfu-util 0.7.

Download firmware to the device:
```
dfu-util -a 0 -D <firmware.bin>
```

Read back (upload) firmware from the device:
```
dfu-util -a 0 -U <readback.bin>
```
