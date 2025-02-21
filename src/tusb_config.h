//
// Copyright (c) 2025 Piers Finlayson <piers@piers.rocks>
//
// Licensed under MIT license - see https://opensource.org/licenses/MIT
//

//
// This is the configuration file for the tinyusb stack for the tinyusb
// vendor example.
//
// This configuration file (or one like it) is required to implement any
// USB host or device using tinyusb, and is included by tinyusb to configure
// the features of the stack.
//

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// Define to use FLASH helper for persistent storage
// #define CFG_TUD_FLASH_XFER_BUFFER   1

// Defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

// RHPort number used for device can be defined by board.mk, default to port 0
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT     0
#endif

// RHPort max operational speed can be defined by board.mk
// Default to Highspeed for MCU with internal HighSpeed PHY (can be port specific), otherwise FullSpeed
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED  OPT_MODE_FULL_SPEED
#endif

// Default is max speed that hardware controller could support with on-chip PHY
#define CFG_TUD_MAX_SPEED    BOARD_TUD_MAX_SPEED

// Device mode with rhport and speed defined by board.mk
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | BOARD_TUD_MAX_SPEED)

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif

// CFG_TUSB_DEBUG is defined by compiler in DEBUG build
#ifdef CFG_TUSB_DEBUG
#undef CFG_TUSB_DEBUG
#endif
#define CFG_TUSB_DEBUG 0  // Increase number to enable tinyusb logging

// Enable device support
#define CFG_TUD_ENABLED       1

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_ENDPOINT0_SIZE    64

//------------- CLASS -------------//

// Vendor specific class configuration
#define CFG_TUD_VENDOR           1
#define CFG_TUD_VENDOR_EP_BUFSIZE  64
#define CFG_TUD_VENDOR_RX_BUFSIZE  64
#define CFG_TUD_VENDOR_TX_BUFSIZE  64

// DFU RT does not required for this project
#define CFG_TUD_DFU_RT           0

// CDC class is not needed
#define CFG_TUD_CDC              0

// MSC class is not needed
#define CFG_TUD_MSC              0

// HID class is not needed
#define CFG_TUD_HID              0

// MIDI class is not needed
#define CFG_TUD_MIDI             0

// Audio class is not needed
#define CFG_TUD_AUDIO            0

// Video class is not needed
#define CFG_TUD_VIDEO            0

// BTH class is not needed
#define CFG_TUD_BTH              0

#ifdef __cplusplus
}
#endif

#endif // TUSB_CONFIG_H