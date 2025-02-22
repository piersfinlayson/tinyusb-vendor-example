# TinyUSB Vendor Example

This project provides a working example of implementing a USB vendor class device using the TinyUSB stack on a Raspberry Pi Pico. It was created to address the lack of comprehensive documentation and examples for implementing custom USB devices with TinyUSB.

## Why This Exists

TinyUSB is a powerful USB stack for embedded devices, but it suffers from limited documentation and a scarcity of real-world examples, particularly for vendor-specific devices. This makes it challenging for developers to:
- Understand how to properly implement USB device descriptors
- Handle both control and bulk transfers effectively
- Manage USB protocol state machines
- Implement reliable error handling and recovery

This example demonstrates these concepts with extensive comments, providing a solid foundation for building your own USB devices.

See [this video](https://youtu.be/f_c9s5aC1No) for an overview of the example, and how to build a vendor device implementation for the Pico using TinyUSB.  (Published 25th February 2025.)

## Features

- Custom USB vendor device implementation
- Support for both control and bulk transfers
- Multicore operation demonstration
- Watchdog implementation
- Example protocol based on xum1541 ([OpenCBM](https://github.com/OpenCBM/OpenCBM) project)
- Support for putting the device into bootloader mode via USB 
- Comprehensive logging vi UART0

## Prerequisites

1. Raspberry Pi Pico C/C++ SDK:
```bash
# On Debian/Ubuntu
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libusb-1.0-0-dev

# Clone Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
cd ..

# Set SDK path (add to ~/.bashrc for permanence)
export PICO_SDK_PATH=/path/to/pico-sdk
```

2. Picotool for flashing:
```bash
sudo apt install picotool
```

## Building

1. Clone this repository:
```bash
git clone https://github.com/piersfinlayson/tinyusb-vendor-example.git
cd tinyusb-vendor-example
```

2. Create build directory:
```bash
mkdir build
cd build
cmake ..
make
```

This will create `tinyusb_vendor_example.uf2` in the build directory.

## Flashing

1. Hold the BOOTSEL button on the Pico while connecting it to USB
2. The Pico will appear as a mass storage device
3. Either:
   - Copy the .uf2 file to the Pico drive, or
   - Use picotool:
     ```bash
     picotool load tinyusb-vendor-example.elf
     picotool reboot
     ```

## Linux udev rules

To access the example device from linux as a non-root user, install the supplied udev rules

```bash
scripts/install-udev-rules.sh
```

## Testing

See [`scripts/README.md`](scripts/README.md) for scripts that will test the device.  For example:

```bash
cd scripts
./pico-init.sh
./pico-info.sh
./pico-write.sh
./pico-read.sh
```

Once the firmware has been flashed, you can enter bootsel mode for firmware programming with:

```bash
cd scripts
./pico-bootsel.sh
```

## USB Device Details

- Vendor ID: 0x1209  (Unassigned)
- Product ID: 0x0f0f (Unassigned)
- Manufacturer: "piers.rocks"
- Product: "tinyusb vendor example"

**Note:** These VID/PID values are not officially allocated and may conflict with other devices.

## Implementation Notes

- Uses USB 1.1 specification
- Bulk endpoint size: 64 bytes
- Control endpoint size: 64 bytes
- Supports multicore operation with watchdog
- Core 1 started and available for custom business logic

For detailed protocol information, see [PROTOCOL.md](PROTOCOL.md)

## Troubleshooting

Comprehensive logging is provided via UART0.

If modifying the USB device descriptor, you'll need to:

### Linux
```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### Windows
Uninstall the device in Device Manager

## License

MIT License - See [LICENSE](LICENSE) for details.

## Author

Piers Finlayson <piers@piers.rocks>

## Contributing

This is an example project, but improvements and bug fixes are welcome. Please submit pull requests or open issues for discussion.
