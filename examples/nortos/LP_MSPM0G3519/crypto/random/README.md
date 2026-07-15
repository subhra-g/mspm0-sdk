## Example Summary

This code example demonstrates how to use the rand entropy module to
extract seed entropy, from which the rand drbg module may be seeded to
enable generation of random numbers.

The entropy moodule extracts 48 bytes of entropy from the TRNG.  Then,
this data is used to seed a DRBG instance.  Then the example loops
and outputs 32 bytes of random data per loop iteration.  Set a breakpoint
on the __NOP in the main loop to observe the values changing in the
randomData buffer.

This example uses the TRNG and AES, but these modules are configured 
and managed by the MSP-CRYPTO-LIB.  No configuration in SysConfig is
required for these modules.

## Peripherals & Pin Assignments

| Peripheral | Pin | Function |
| --- | --- | --- |
| DEBUGSS | PA20 | Debug Clock |
| DEBUGSS | PA19 | Debug Data In Out |

## BoosterPacks, Board Resources & Jumper Settings

| Pin | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| --- | --- | --- | --- | --- |
| PA20 | DEBUGSS | SWCLK | N/A | <ul><li>PA20 is used by SWD during debugging<br><ul><li>`J14 15:16 ON` Connect to XDS-110 SWDIO while debugging<br><li>`J14 15:16 OFF` Disconnect from XDS-110 SWDIO if using pin in application</ul></ul> |
| PA19 | DEBUGSS | SWDIO | N/A | <ul><li>PA19 is used by SWD during debugging<br><ul><li>`J14 13:14 ON` Connect to XDS-110 SWDIO while debugging<br><li>`J14 13:14 OFF` Disconnect from XDS-110 SWDIO if using pin in application</ul></ul> |

## Example Usage
Compile, load and run the example.  Set break points at the trap loops at the end of main() to verify correct execution.
