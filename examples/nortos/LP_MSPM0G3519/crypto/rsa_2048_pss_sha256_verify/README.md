## Example Summary

This code example demonstrates how to use the RSA verify function
to verify an RSA digital signature over a message.

This example uses the RSA PSS padding mode from RFC 8017.

This example uses SHA-256 for the message pre-hash, as well as the hash function 
for the MGF inside of the PSS padding verification.

This example uses the single-call approach to RSA verification.

**NOTE**: the RSA public exponent is always assumed to be 65537.

The routine is expected to return a VALID status.  
To test the INVALID signature case, change
any part of the message and re-run the example to 
confirm that INVALID is reported.

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
