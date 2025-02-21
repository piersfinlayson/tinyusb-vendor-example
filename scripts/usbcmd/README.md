

# usbcmd - USB Command Line Tool

A command-line tool for sending USB control and bulk transfers to devices. This tool provides a simple interface for USB device communication, particularly useful for testing and debugging USB devices.

## Features

- List all connected USB devices
- Send USB control transfers (IN/OUT)
- Send USB bulk transfers (IN/OUT)
- Hexadecimal or decimal input for all numeric parameters
- Clear error reporting

## Prerequisites

- Python 3 (any modern version)
- libusb 1.0
- pyusb package

## Installation

1. Install libusb development package:
   ```bash
   # Ubuntu/Debian
   sudo apt install libusb-1.0-0-dev

   # Fedora
   sudo dnf install libusb1-devel

   # Arch Linux
   sudo pacman -S libusb
   ```

2. Install Python dependencies:
   ```bash
   pip install pyusb
   ```

## Usage

### Basic Command Structure
```bash
./usbcmd.py -v VENDOR_ID -p PRODUCT_ID COMMAND [OPTIONS]
```

### Commands

1. List USB Devices
   ```bash
   ./usbcmd.py list
   ```

2. Control Transfer
   ```bash
   ./usbcmd.py -v VID -p PID control [in|out] -t TYPE -r REQUEST -v VALUE -i INDEX [-d DATA] [-l LENGTH]
   ```

3. Bulk Transfer
   ```bash
   ./usbcmd.py -v VID -p PID bulk [in|out] ENDPOINT [-d DATA] [-l LENGTH]
   ```

### Parameters

- `-v`, `--vendor-id`: USB vendor ID (hex with 0x prefix or decimal)
- `-p`, `--product-id`: USB product ID (hex with 0x prefix or decimal)
- `-t`, `--type`: Request type for control transfers
- `-r`, `--request`: Request number for control transfers
- `-v`, `--value`: Value field for control transfers
- `-i`, `--index`: Index field for control transfers
- `-d`, `--data`: Data to send (hex format with 0x prefix)
- `-l`, `--length`: Length of data to receive (default: 64)

### Examples

1. List all USB devices:
   ```bash
   ./usbcmd.py list
   ```

2. Send a control OUT transfer:
   ```bash
   ./usbcmd.py -v 0x1209 -p 0x0f0f control out -t 0x40 -r 0x42 -v 0 -i 0
   ```

3. Send a control OUT transfer with data:
   ```bash
   ./usbcmd.py -v 0x1209 -p 0x0f0f control out -t 0x40 -r 0x42 -v 0 -i 0 -d 0x0102
   ```

4. Receive data with control IN transfer:
   ```bash
   ./usbcmd.py -v 0x1209 -p 0x0f0f control in -t 0xC0 -r 0x42 -v 0 -i 0 -l 64
   ```

5. Send data with bulk OUT transfer:
   ```bash
   ./usbcmd.py -v 0x1209 -p 0x0f0f bulk out 0x01 -d 0x48656C6C6F
   ```

6. Receive data with bulk IN transfer:
   ```bash
   ./usbcmd.py -v 0x1209 -p 0x0f0f bulk in 0x82 -l 64
   ```

## Permissions

By default, Linux systems restrict access to USB devices. You have two options:

1. Run the tool with sudo:
   ```bash
   sudo ./usbcmd.py ...
   ```

2. Create a udev rule (recommended for development):
   ```bash
   # Create a file: /etc/udev/rules.d/99-usb-device.rules
   SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="5740", MODE="0666"
   
   # Reload rules
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```
   Replace the vendor and product IDs with your device's IDs.

## Troubleshooting

1. "Device not found" error:
   - Check if the device is connected using `lsusb`
   - Verify vendor and product IDs
   - Check USB cable
   - Try unplugging and replugging the device

2. Permission denied:
   - Run with sudo
   - Set up udev rules as described above
   - Check device permissions with `ls -l /dev/bus/usb/...`

3. Transfer errors:
   - Verify endpoint addresses
   - Check if the device is in the correct state
   - Verify transfer parameters (length, direction)
   - Check if another program is using the device

## Limitations

- No interrupt or isochronous transfer support
- No configuration or interface selection
- No support for composite devices
- Limited error recovery

## Contributing

Feel free to open issues or submit pull requests for improvements and bug fixes.

## License

This tool is released under the MIT License. See the LICENSE file for details.