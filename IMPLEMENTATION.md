# TinyUSB Vendor Implementation Guide

This guide explains the key files and functions required to implement a USB vendor device using TinyUSB. While based on a Raspberry Pi Pico implementation, the concepts apply broadly to other TinyUSB-supported platforms.

## Core Files

### main.c
The main implementation file containing the device logic and USB callbacks.

Key Functions:
- `main()`: Initializes the Pico, watchdog, and TinyUSB stack
- `tud_vendor_rx_cb()`: Handles bulk write transfers from host
- `tud_vendor_tx_cb()`: Called when bulk data has been sent to host
- `tud_vendor_control_xfer_cb()`: Processes control transfers
- `maybe_send_data()`: Used to send bulk data from main loop
- `tud_mount_cb()`, `tud_umount_cb()`: Device mount/unmount handlers
- `tud_suspend_cb()`, `tud_resume_cb()`: Power management handlers

### usb_desc.c 
Contains all USB descriptors and descriptor callbacks.

Key Components:
- `desc_device`: Device descriptor structure defining USB version, VID/PID, etc.
- `desc_configuration`: Configuration descriptor including interface and endpoint definitions
- `string_desc_arr`: String descriptors for manufacturer, product, etc.

Key Functions:
- `tud_descriptor_device_cb()`: Returns device descriptor
- `tud_descriptor_configuration_cb()`: Returns configuration descriptor
- `tud_descriptor_string_cb()`: Handles string descriptor requests

### tusb_config.h
Configuration header for TinyUSB stack.

Essential Defines:
```c
#define CFG_TUD_ENABLED       1

#define CFG_TUD_VENDOR              1  // Enable vendor class
#define CFG_TUD_VENDOR_RX_BUFSIZE  64 // RX buffer size
#define CFG_TUD_VENDOR_TX_BUFSIZE  64 // TX buffer size
```

### include.h
Project-specific definitions.

Essential Defines:
```c
// USB Device Info
#define EXAMPLE_VID           0xXXXX  // Your Vendor ID
#define EXAMPLE_PID           0xYYYY  // Your Product ID
#define MANUFACTURER          "your-name"
#define PRODUCT               "your-product"

// Endpoint Configuration
#define ENDPOINT_BULK_SIZE    64
#define MAX_ENDPOINT0_SIZE    64
```

## Implementation Steps

1. **Device Descriptors**
   - Define device descriptor in `usb_desc.c`
   - Implement descriptor callbacks
   - Configure VID/PID and strings

2. **TinyUSB Configuration**
   - Enable tinyusb device support and vendor class in `tusb_config.h`
   - Set buffer sizes
   - Configure endpoint counts

3. **Main Loop**
   - Initialize TinyUSB with `tusb_init()`
   - Call `tud_task()` regularly to process USB events

4. **Transfer Handling**
   - Implement control transfer callback
   - Implement bulk transfer callbacks
   - Manage protocol state machine

5. **Protocol Implementation**
   - Define command structure
   - Implement command parsing
   - Handle data transfers
   - Send status responses

## Key Implementation Notes

1. **Control Transfers**
   - Always respond in SETUP stage
   - Use `tud_control_xfer()` to send response
   - Check transfer direction (IN/OUT)

2. **Bulk Transfers**
   - Maintain transfer state machine
   - Handle partial transfers
   - Manage buffer space
   - Use `tud_vendor_write()` to send data to host on IN endpoint

3. **Buffer Management**
   - Respect endpoint sizes
   - Consider using ring buffers
   - Handle buffer overflow

4. **Error Handling**
   - Validate commands
   - Send error status
   - Recover from errors

## Common Pitfalls

1. Not calling `tud_task()` frequently enough
2. Incorrect endpoint/buffer sizes
3. Missing descriptor callbacks
4. Not handling partial transfers
5. Buffer overflow in string descriptors
6. Changing USB device properties and not clearing device in OS

## Debugging Tips

1. Use UART logging for USB events
2. Monitor device enumeration
3. Test all transfer types
4. Verify descriptor values

## Related Documentation

- [TinyUSB Documentation](https://docs.tinyusb.org)
- [USB 2.0 Specification](https://www.usb.org/document-library/usb-20-specification)
- [Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/pico-sdk)