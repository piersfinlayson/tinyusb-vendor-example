#!/bin/bash
usbcmd/usbcmd.py -v 0x1209 -p 0x0f0f control in -t 0xa1 -r 0x04 -v 0 -i 0 -l 8