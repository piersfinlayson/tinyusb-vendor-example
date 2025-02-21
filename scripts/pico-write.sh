#!/bin/bash

# 0x09 is WRITE, 0x10 is protocol, 0x02 | (0x00 << 8) is amount of data to write
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f bulk out 0x04 -d 0x09100200

# Now write 2 bytes (both 0x00)
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f bulk out 0x04 -d 0x0000 

# Read in 3 byte status response after READ
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f bulk in 0x83 -l 3