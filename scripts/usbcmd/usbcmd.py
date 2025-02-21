#!/usr/bin/env python3

#
# Copyright (c) 2025 Piers Finlayson <piers@piers.rocks>
#
# Licensed under MIT license - see https://opensource.org/licenses/MIT
#

import argparse
import sys
import usb.core
import usb.util

def decode_and_print_data(data):
    """Print received data in both hex and ASCII format."""
    # Print hex representation
    hex_str = ''.join(f'{x:02x}' for x in data)
    print(f"Received (hex): 0x{hex_str}")
    
    # Print ASCII representation, replacing non-printable characters with dots
    ascii_str = ''.join(chr(x) if 32 <= x <= 126 else '.' for x in data)
    print(f"Received (ASCII): {ascii_str}")

def parse_int(s: str) -> int:
    """Parse string as hex (if starts with 0x) or decimal."""
    if s.lower().startswith('0x'):
        return int(s, 16)
    return int(s)

def parse_data(data: str) -> bytes:
    """Convert 0x hex string to bytes."""
    if not data:
        return b''
    if not data.lower().startswith('0x'):
        raise ValueError("Data must start with 0x")
    hex_str = data[2:]  # Remove 0x
    if len(hex_str) % 2 != 0:
        raise ValueError("Hex string must have even length")
    try:
        return bytes.fromhex(hex_str)
    except ValueError:
        raise ValueError("Invalid hex string")

def list_devices():
    """List all USB devices."""
    devices = usb.core.find(find_all=True)
    for dev in devices:
        print(f"ID {dev.idVendor:04x}:{dev.idProduct:04x}")

def find_device(vendor_id: int, product_id: int) -> usb.core.Device:
    """Find USB device by vendor and product ID."""
    device = usb.core.find(idVendor=vendor_id, idProduct=product_id)
    if device is None:
        raise ValueError(f"Device {vendor_id:04x}:{product_id:04x} not found")
    return device

def setup_device(device):
    """Setup device for use."""
    # Get first configuration
    cfg = device.get_active_configuration()
    if cfg is None:
        # If no active config, set the first one
        device.set_configuration()
        cfg = device.get_active_configuration()
    
    # Get first interface
    interface = cfg[(0,0)].bInterfaceNumber

    # Check if kernel driver is attached
    if device.is_kernel_driver_active(interface):
        device.detach_kernel_driver(interface)
        was_kernel_driver_active = True
    else:
        was_kernel_driver_active = False

    # Claim interface
    usb.util.claim_interface(device, interface)
    
    return interface, was_kernel_driver_active

def cleanup_device(device, interface, reattach_kernel_driver):
    """Cleanup device after use."""
    try:
        usb.util.release_interface(device, interface)
        if reattach_kernel_driver:
            device.attach_kernel_driver(interface)
    except:
        pass  # Best effort cleanup

def do_control(args):
    """Execute control transfer."""
    device = find_device(args.vendor_id, args.product_id)
    data = parse_data(args.data) if args.data else b''
    
    #print(f"DEBUG: Sending control transfer:")
    #print(f"  bmRequestType: 0x{args.type:02x}")
    #print(f"  bRequest: 0x{args.request:02x}")
    #print(f"  wValue: 0x{args.value:04x}")
    #print(f"  wIndex: 0x{args.index:04x}")
    #print(f"  Direction: {'IN' if args.direction == 'in' else 'OUT'}")
    #if args.direction == 'out':
    #    print(f"  Data: {data.hex() if data else 'None'}")
    #else:
    #    print(f"  Length: {args.length}")
    
    if args.direction == 'out':
        device.ctrl_transfer(args.type, args.request, args.value, args.index, data)
    else:  # in
        data = device.ctrl_transfer(args.type, args.request, args.value, args.index, args.length)
        decode_and_print_data(data)

def do_bulk(args):
    """Execute bulk transfer."""
    device = find_device(args.vendor_id, args.product_id)
    
    # Setup device
    interface, was_kernel_driver_active = setup_device(device)
    
    try:
        if args.direction == 'out':
            data = parse_data(args.data) if args.data else b''
            #print(f"DEBUG: Sending bulk OUT transfer:")
            #print(f"  Endpoint: 0x{args.endpoint:02x}")
            #print(f"  Data (hex): {data.hex()}")
            #print(f"  Data length: {len(data)} bytes")
            written = device.write(args.endpoint, data)
            #print(f"  Bytes written: {written}")
        else:  # in
            # Need a bit of a timeout to give the device time to send the
            # response
            data = device.read(args.endpoint, args.length, timeout=500)
            decode_and_print_data(data)
    finally:
        # Always cleanup
        cleanup_device(device, interface, was_kernel_driver_active)

def main():
    parser = argparse.ArgumentParser(description='USB Control Tool')
    parser.add_argument('-v', '--vendor-id', type=parse_int, help='Vendor ID (hex with 0x or decimal)')
    parser.add_argument('-p', '--product-id', type=parse_int, help='Product ID (hex with 0x or decimal)')

    subparsers = parser.add_subparsers(dest='command', help='Command')

    # List command
    list_parser = subparsers.add_parser('list', help='List USB devices')

    # Control transfer command
    control_parser = subparsers.add_parser('control', help='Control transfer')
    control_parser.add_argument('direction', choices=['in', 'out'], help='Transfer direction')
    control_parser.add_argument('-t', '--type', type=parse_int, required=True, help='Request type (hex with 0x or decimal)')
    control_parser.add_argument('-r', '--request', type=parse_int, required=True, help='Request (hex with 0x or decimal)')
    control_parser.add_argument('-v', '--value', type=parse_int, required=True, help='Value (hex with 0x or decimal)')
    control_parser.add_argument('-i', '--index', type=parse_int, required=True, help='Index (hex with 0x or decimal)')
    control_parser.add_argument('-d', '--data', help='Data to send (hex with 0x)')
    control_parser.add_argument('-l', '--length', type=int, default=64, help='Length for IN transfer')

    # Bulk transfer command
    bulk_parser = subparsers.add_parser('bulk', help='Bulk transfer')
    bulk_parser.add_argument('direction', choices=['in', 'out'], help='Transfer direction')
    bulk_parser.add_argument('endpoint', type=parse_int, help='Endpoint (hex with 0x or decimal)')
    bulk_parser.add_argument('-d', '--data', help='Data to send (hex with 0x)')
    bulk_parser.add_argument('-l', '--length', type=parse_int, default=64,  help='Length for IN transfer (hex with 0x or decimal)')

    args = parser.parse_args()

    try:
        if args.command == 'list':
            list_devices()
        elif args.command == 'control':
            do_control(args)
        elif args.command == 'bulk':
            do_bulk(args)
        else:
            parser.print_help()
            sys.exit(1)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    except usb.core.USBError as e:
        print(f"USB Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()