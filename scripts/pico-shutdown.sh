#!/bin/bash

# -t 0x21 is 0x00 (OUT) | 0x20 (vendor specific) | 0x01 (recipient interface)
# -r 0x03 is the request ID for SHUTDOWN
# -v is the value (ignored)
# -i is the itnerface
# -l 8 is the expected length of the response
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f control out -t 0x21 -r 0x03 -v 0 -i 0 -l 8
