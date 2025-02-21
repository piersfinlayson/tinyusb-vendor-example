#!/bin/bash

# 0x08 is READ, 0x10 is protocol, 0x40 | (0x00 << 8) is amount of data to read
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f bulk out 0x04 -d 0x08104000

# Read in the 0x40 (64) bytes expected
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f bulk in 0x83 -l 0x40

# Read in 3 byte status response after READ
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f bulk in 0x83 -l 3