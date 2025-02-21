//
// Copyright (c) 2025 Piers Finlayson <piers@piers.rocks>
//
// Licensed under MIT license - see https://opensource.org/licenses/MIT
//

//
// Main header file for the tinyusb vendor example.
//

//
// USB device descriptor information
//

// PID and VID for this USB device.  If you change the USB device descriptor,
// you will either need to change the VID/PID or tell the OS to forget the
// device:
//
// Linux - ```sudo udevadm control --reload-rules && sudo udevadm trigger```
// Windows - Uninstall the device in Device Manager
//
// Note these VID/PID are not officially allocated, so may clash with real
// devices.  Use at your own risk
#define EXAMPLE_VID 0x1209
#define EXAMPLE_PID 0x0f0f

// Maximum packet sizes for the endpoints.  64 is a very standard value
#define MAX_ENDPOINT0_SIZE  64
#define ENDPOINT_BULK_SIZE  64

// Strings for the USB device descriptor
#define MANUFACTURER  "piers.rocks"
#define PRODUCT       "tinyusb vendor example"
#define SERIAL        "000"

// Indexes for the strings in the USB device descriptor
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

// Interfaces for the USB device descriptor
enum {
    ITF_NUM_VENDOR = 0,
    ITF_NUM_TOTAL
};

// These could be 0x01 and 0x02 (with the IN ORed with 0x80).  I'm setting
// them to 0x83 and 0x04 to replicate another device.
#define BULK_IN_ENDPOINT_DIR   0x83
#define BULK_OUT_ENDPOINT_DIR  0x04

//
// Logging macros
//

#define INFO(fmt, ...) printf("core%d: " fmt "\n", get_core_num(), ##__VA_ARGS__)
#ifdef DEBUG
#define DEBUG(...)  INFO(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

// How often to log in the loops - this is the number of interations to use as
// a period
#define LOG_INTERVAL_COUNT 5000000

//
// tinyusb vendor example protocol definitions
//

// Supported IN control transfer requests 
#define CTRL_ECHO              0x00
#define CTRL_INIT              0x01
#define CTRL_RESET             0x02
#define CTRL_SHUTDOWN          0x03
#define CTRL_ENTER_BOOTLOADER  0x04
#define CTRL_GITREV            0x06
#define CTRL_GCCVER            0x07
#define CTRL_SDKVER            0x08

// Supported write_bulk protocol commands
#define CMD_NONE                   0
#define CMD_READ                   8
#define CMD_WRITE                  9

// Supported command protocols
#define PROTO_DEFAULT              16

// Nmber of bytes in a write_bulk command
#define COMMAND_LEN                4

// Number of bytes in a status response
#define STATUS_LEN                 3

// Status codes for the first byte of the status response
#define STATUS_BUSY                1
#define STATUS_READY               2
#define STATUS_ERROR               3

