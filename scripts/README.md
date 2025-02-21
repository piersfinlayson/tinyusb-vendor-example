# scripts

This directory contains scripts for using tinyusb-vendor-example.
* [`usbcmd/usbcmd.py`](usbcmd/usbcmd.py) - a utility that allows you to send control transfers, and send and receive bulk dawta
* `pico-*.sh` - bash scripts that use usbcmd.py to perform common actions

See [usbcmd/README.md](usbcmd/README.md) for instructions on using `usbcmd.py`.

The `pico.*.sh` commands are described below:

### pico-bootsel.sh

```./pico-bootsel.sh```

Puts the device into bootsel (DFU, bootloader, programming mode).  You can then write a new firmware image by running the following commands from your build directory:

```bash
picotool load tinyusb-vendor-example.elf
picotool reboot
```

Or, alternatively, drag `tinyusb-vendor-example.uf2` onto the mounted Pico filesystem.

### pico-init.sh

```./pico-init.sh```

Sends a control tranfer to the device to initialize protocol support within it, and the prints out the response bytes as hex.  Do this before running subsequent commands, or to clear up bad state in the device.

### pico-write.sh

```./pico-write.sh```

Sends a WRITE command to the device, telling it to expect some bytes of data, and then sends the expected number of bytes of data.

### pico-read.sh

```./pico-read.sh```

Sends a READ command to the device, telling it to return some bytes of data, and then reads those bytes of data.

### pico-info.sh

```./pico-info.sh```

Sends control transfers to the device to query device information (git revision, gcc version, and Pico SDk version).  Outputs the responses as ASCII strings.