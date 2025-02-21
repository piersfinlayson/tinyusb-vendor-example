#!/bin/bash

# -t 0xa1 is 0x80 (IN) | 0x20 (vendor specific) | 0x01 (recipient interface)
# -r 0x06 is the request ID for git revision
# -v is the value (ignored)
# -i is the itnerface
# -l 8 is the expected length of the response
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f control in -t 0xa1 -r 0x06 -v 0 -i 0 -l 8

# -r 0x07 is the request ID for GCC version
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f control in -t 0xa1 -r 0x07 -v 0 -i 0 -l 8

# -r 0x08 is the request ID for Pico SDK version
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f control in -t 0xa1 -r 0x08 -v 0 -i 0 -l 8
