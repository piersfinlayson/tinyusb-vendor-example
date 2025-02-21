#!/bin/bash

# -t 0xa1 is 0x80 (IN) | 0x20 (vendor specific) | 0x01 (recipient interface)
# -r 0x01 is the request ID for INIT
# -v is the value (ignored)
# -i is the itnerface
# -l 8 is the expected length of the response
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f control in -t 0xa1 -r 0x01 -v 0 -i 0 -l 8