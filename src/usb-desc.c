//
// Copyright (c) 2025 Piers Finlayson <piers@piers.rocks>
//
// Licensed under MIT license - see https://opensource.org/licenses/MIT
//

//
// USB descriptor for the tinyusb vendor example 
//

#include "include.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "device/usbd.h"

// Device descriptor.
// For a vendor device ths is simple. DevicClass is 0xff and SubClass and
// DeviceProtocol are 0x00.
// However mixing classes (for example including a CDC as well), you must use
// TUSB_CLASS_MISC, MISC_SUBCLASS_COMMON and MISC_PROTOCOL_IAD.
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0110,  // USB 1.1
    .bDeviceClass       = TUSB_CLASS_VENDOR_SPECIFIC,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = MAX_ENDPOINT0_SIZE,
    .idVendor           = EXAMPLE_VID,
    .idProduct          = EXAMPLE_PID,
    .bcdDevice          = 0x0001,
    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = 0x01
};

// Callback which is invoked when GET DEVICE DESCRIPTOR is received
uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// Configuration descriptor
// Just need the TUD_CONFIG_DESCRIPTOR, and a TUD_VENDOR_DESCRIPTOR
// You can add other descriptors here if you mix classes (e.g. add
// a CDC descriptor as well to add a communications device)
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)
uint8_t static desc_configuration[] = {
    // Configuration descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),

    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, BULK_OUT_ENDPOINT_DIR, BULK_IN_ENDPOINT_DIR, ENDPOINT_BULK_SIZE),
};

// String descriptors
char const* string_desc_arr[] = {
    [STRID_LANGID]      = (const char[]) { 0x09, 0x04 },  // Supported language ID - English (US)
    [STRID_MANUFACTURER] = MANUFACTURER,
    [STRID_PRODUCT]      = PRODUCT,
    [STRID_SERIAL]       = SERIAL,
};

// Callback invoked when GET CONFIGURATION DESCRIPTOR is received
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    static_assert(sizeof(desc_configuration) == CONFIG_TOTAL_LEN, "Configuration descriptor size mismatch");

    return desc_configuration;
}


// Callback invoked when GET STRING DESCRIPTOR is received.
// Of course, you can decide to supply dynamically calculated values here,
// although those changing over time may cause problems with OSes that have
// cached the values for your VID/PID pair.
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    static uint16_t _desc_str[32 + 1];
    (void) langid;
    size_t chr_count;
    const char *str;

    switch (index) {
        case STRID_LANGID:
            memcpy(&_desc_str[1], string_desc_arr[0], 2);
            chr_count = 1;
            break;

        case STRID_SERIAL:
        case STRID_MANUFACTURER:
        case STRID_PRODUCT:
            str = string_desc_arr[index];
            chr_count = strlen(str);

            // Ensure we don't overwrite the buffer
            size_t const max_count = (sizeof(_desc_str) / sizeof(_desc_str[0])) - 1;
            if (chr_count > max_count) 
            {
                chr_count = max_count;
            }

            // Convert to UTF-16
            for (size_t ii = 0; ii < chr_count; ii++) {
                _desc_str[1 + ii] = str[ii];
            }
            break;
            
        default:
            return NULL;
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}
