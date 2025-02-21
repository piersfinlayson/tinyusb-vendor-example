//
// Copyright (c) 2025 Piers Finlayson <piers@piers.rocks>
//
// Licensed under MIT license - see https://opensource.org/licenses/MIT
//

//
// This example demonstrates a simple USB device that uses the tinyusb stack
// to implement a vendor class device.  The device has a single interface, and
// supports both control and bulk transfers.
//
// While the protocol this example implements can be considered arbitrary, it
// actually is a subset of the protocol used by the xum1541 project, which
// is part of OpenCBM  - see https://github.com/OpenCBM/OpenCBM
//

// Pico header files
#include "pico/stdlib.h"        // Standard pico include file
#include "pico/bootrom.h"       // To allow us to reboot the device into BOOTSEL mode
#include "pico/multicore.h"     // For multicore support
#include "hardware/watchdog.h"  // For watchdog support

// tinusb header files
#include "tusb.h"               // Standard tinyusb header file
#include "bsp/board_api.h"      // tinyusb's Pico specific header file

// Our own header files
#include "include.h"

// Forward declaration of functions later in main.c that we need to call from
// main()
void core1(void);
void example_tight_loop_contents(char *loop_name);
void maybe_send_data(void);
void enter_bootloader(void);

// Our main function, which
// - Sets up the pico, a watchdog and the tinyusb stack
// - Runs a loop scheduling tinyusb, feeding the watchdog and implementing our
//   sample protocol
void main(void) {
    // Initialize the Pico
    stdio_init_all();
    stdio_set_driver_enabled(&stdio_uart, true);

    INFO("-----");
    INFO("tinyusb vendor example started");

    // Start a watchdog
    watchdog_enable(5000, 1);  // Timeout in ms

    if (watchdog_caused_reboot()) {
        INFO("Watchdog caused last reboot");
    }

    // Create a new task on core 1.
    //
    // This is a no-op in this example, but you could in theory run your
    // business logic on one core, and schedule tinyusb on the other core (so
    // long as you have some thread safe mechanism to communicate between
    // them).
    //
    // Or run usb and business logic on one task, and other tasks, such as
    // WiFi handling, on the other core.
    //
    // Just remember, if you use a watchdog, to feed it with with
    // watchdog_update from both cores.
    multicore_launch_core1(core1);

    // Initialize tinyusb
    board_init();  // This is a Pico specific tinyusb board init function
    tusb_init();   // This calls tud_init() assuming tusb_config.h is set up correctly

    // Now enter our main loop, running forever
    while (true) {
        // Makes this tight loop searchable (even though it's not strictly a
        // tight loop because it does some work)
        example_tight_loop_contents("main loop");

        // Schedule tinyusb device stack to allow it to do some work.
        // While incoming USB packets are received by tinyusb via interrupts,
        // it doesn't call our callbacks via interrupts.  Instead it queues
        // them up and schedules them from within tud_task().  Hence if you
        // don't call tud_task(), USB won't work!
        tud_task();

        // See if we were requested to send data, and if so, send it
        maybe_send_data();

        // Feed the watchdog
        watchdog_update();
    }
}

//
// Our sample protocol handling code
//

// Some statics to support reading/writing arbitrary amounts of data from/to
// the host in response to a WRITE or READ command (coming in from
// write_bulk).
//
// See tud_vendor_rx_cb() for more details
//
// expected_data_len of 0 means we expect a command as the next transfer.
//
// If expected_data_len is non-zero, we are expecting to receive or send
// data.  We expect a remainder of (expected_data_len - handled_data_len)
// bytes.
//
// If handled_data_len == expected_data_len we're done on this command 
//
// These are
// statics, so they retain their values across callbacks (but are not
// thread-safe, if running this example on multiple cores).
static uint16_t expected_data_len = 0;
static uint16_t handled_data_len = 0;

// Static holding the current command (received from write_bulk) being
// executed
static uint8_t current_command = CMD_NONE;

// Buffer used to return a status. As this is a static, and we could receive
// and handle another command before the usb stack manages to send the status
// we shouldn't reuse this buffer between commands, without providing some
// sort of checking (for example in tud_vendor_rx_cb()), but we will anyway.
static uint8_t status[STATUS_LEN];

// Used by our protocol handling to reset data read once we've read/written
// the data associated with a WRITE command
void reset_data(void) {
    expected_data_len = 0;
    handled_data_len = 0;
}

// Send a status back in response to a bulk command.  The format of the status
// is:
// byte 0 - a 1 byte status value (STATUS_BUSY, STATUS_READY or STATUS_ERROR)
// byte 1 - low order byte of data length
// byte 2 - high order byte of data length
void send_status_response(uint8_t status_val, uint16_t data_len) {
    static_assert(STATUS_LEN == 3);
    
    // Fill in the status 
    status[0] = status_val;
    status[1] = (uint8_t)(data_len & 0xff);
    status[2] = (uint8_t)(data_len >> 8);
    INFO("All data received - send status response: 0x%02x 0x%02x 0x%02x", status[0], status[1], status[2]);

    // Send it - and flush the write buffer to ensure it gets sent immediately
    tud_vendor_write(status, STATUS_LEN);
    tud_vendor_write_flush();
}

// Used to send data from within our main loop, if we received a command
// asking us to send data.  We'll send up to the amount tinyusb will let us
// (and that we need to send), every time we're called.
void maybe_send_data(void) {
    // We're going to use this static to send data.  This is a bit naughty
    // as it might get overwritten by a subsequent send if tinyusb doesn't
    // send it quickly.  In reality this is unlikely to be a problem for
    // small amounts of data at low speeds. 
    static uint8_t send_buffer[64];

    // Local variables used to figure out how many bytes to send this time
    uint32_t max_bytes_to_send;
    uint16_t want_to_send;
    uint16_t try_to_send;

    // Number of bytes sent
    uint32_t sent;
    
    if (current_command == CMD_READ) {
        // We are expecting to send data

        // We can only send as many bytes as tinyusb will let us (based on its
        // internal buffer).
        max_bytes_to_send = tud_vendor_write_available();

        // We have this many bytes to actually send
        want_to_send = expected_data_len - handled_data_len;
        
        // Get the minimum of the two values
        if (want_to_send > max_bytes_to_send) {
            try_to_send = max_bytes_to_send;
        } else {
            try_to_send = want_to_send;
        }

        // Now we're going to use a stack buffer to send the data.  We will
        // cap this at 64 bytes to avoid blowing the stack.  In reality, the
        // max endpoint buf size is likely to be 64 bytes, so all should be
        // good.
        //
        // Note that using a stack buffer is a bit naughty.  The USB stack
        // may 
        if (try_to_send > 64) {
            INFO("We wanted to send more than 64 bytes (%d), capping at 64", try_to_send);
            try_to_send = 64;
        }

        INFO("Trying to send %d bytes", try_to_send);

        // Send the data - we'll just send ascii 'x' characters
        memset(send_buffer, 'x', try_to_send);
        sent = tud_vendor_write(send_buffer, try_to_send);
        tud_vendor_write_flush();
        INFO("Actually sent %d bytes", sent);
        
        // Now update the data statics
        handled_data_len += sent;

        if (handled_data_len >= expected_data_len) {
            // No status after READ completes

            // Now we've sent all the data, reset back to waiting for a
            // command
            reset_data();
            current_command = CMD_NONE;
        }
    }

    return;
}

// Used by tud_vendor_control_xfer_cb() to initialize protocol handling on
// a CTRL_INIT command
void init_protocol_handling(void) {
    current_command = CMD_NONE;
    reset_data();
}

//
// tinyusb callbacks
//

// USB device has been mounted.  This means the host has
// - Detected it
// - Read its device descriptor OK
// - Read its configuration descriptor OK (this includes interface and
//   endpoint information)
// - Set a configuration (we only have 1 configuration)
void tud_mount_cb(void) {
    INFO("Device mounted");

    // We will reset our protocol handling support
    init_protocol_handling();
}

// USB device has been unmounted.  There are a few reasons this might happen:
// - Physical disconnection of the USB cable
// - Host sending a USB reset
// - Host entering a suspend state
// - Device-initiated detachment
void tud_umount_cb(void) {
    INFO("Device unmounted");

    // We will reset our protocol handling support
    init_protocol_handling();
}

// USB device has been suspended.  For example, because:
// - The host computer is going into sleep/suspend mode
// - When USB selective suspend is triggered (power saving feature where the
//   host suspends individual USB devices)
//
// Requirements on the device:
// - The device must reduce current draw to <= 2.5mA
// - All USB transactions stop
// - Device should put peripherals into low power mode
// - If remote_wakeup_en is true, the device can wake the host using remote
//   wakeup feature
void tud_suspend_cb(bool remote_wakeup_en) {
    INFO("Device suspended, remote wakeup %s", remote_wakeup_en ? "enabled" : "disabled");

    // We will reset our protocol handling support
    init_protocol_handling();
}

// USB device has been resumed.  For example, because:
// - The host has waken up from sleep
// - The host explicitly brought the device out of selective suspend
// - The device successfully triggered a remote wakeup
void tud_resume_cb(void) {
    INFO("Device resumed");

    // We will reset our protocol handling support
    init_protocol_handling();
}

// This callback handles write_bulk transfers.
//
// You can send back using tud_vendor_write().
void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
    // In our protocol, we expect a 4 byte command followed by an optional
    // number of bytes, as indicated in the 4 byte command.
    //
    // Bear in mind this protocol in entirely arbitary - you can implement
    // whatever protocol you would lile.
    //
    // The format of the command is:
    // byte 0 - command
    // byte 1 - indicates protocol to use
    // byte 2 - length of data which follows (low order byte)
    // byte 3 - length of data which follows (high order byte)
    //
    // After the command, plus any data, has been received, we respond with
    // a 3 byte status.
    //
    // Note that the command and any data are expected to come in multiple
    // callbacks, and the data may well come in several itself (as our maximum
    // endpoint bulk size is 64).
    
    // Local variables used to handle the command protocol
    uint8_t cmd;
    uint8_t proto;

    // Check the interface
    if (itf == ITF_NUM_VENDOR) {
        if (current_command == CMD_NONE) {
            // We are expecting a new command

            if (bufsize == COMMAND_LEN) {
                // Get the command and protocol
                cmd = buffer[0];
                proto = buffer[1];

                // Handle the specific command
                switch (buffer[0]) {
                    case CMD_WRITE:
                        // Get the expected data length
                        expected_data_len = buffer[2] | (buffer[3] << 8);
                        handled_data_len = 0;

                        INFO("Got WRITE command, expecting to receive %d bytes of data", expected_data_len);
    
                        if (expected_data_len == 0) {
                            // No data expected - return status now (all zeros)
                            memset(status, 0, STATUS_LEN);
                            status[0] = STATUS_READY;

                            INFO("Send status response: 0x%02x 0x%02x 0x%02x", status[0], status[1], status[2]);

                            tud_vendor_write(status, STATUS_LEN);
                            tud_vendor_write_flush();
                        } else {
                            // Set the current command, so we know to expect
                            // data subsequqently
                            current_command = buffer[0];
                        }
                        break;

                    case CMD_READ:
                        // Get the expected data length
                        expected_data_len = buffer[2] | (buffer[3] << 8);
                        handled_data_len = 0;

                        INFO("Got READ command, expecting to send %d bytes of data", expected_data_len);

                        if (expected_data_len > 0) {
                            // Set the current command, so we know to send
                            // data from within our main loop
                            current_command = buffer[0];
                        } else {
                            // No bytes requested, so nothing to do

                            // Don't send back a status for a READ

                            // Reset back to waiting for a command
                            reset_data();
                            current_command = CMD_NONE;
                        }
                        break;
    
                    default:
                        INFO("Unsupported command: 0x%02x 0x%02x 0x%02x 0x%02x", buffer[0], buffer[1], buffer[2], buffer[3]);
                        send_status_response(STATUS_ERROR, 0);
                        break;
                }
            } else {
                INFO("Unexpected command length: %d", bufsize);
                send_status_response(STATUS_ERROR, 0);
                return;
            }
        } else {
            // We are expecting to send or receive data
            switch (current_command) {
                case CMD_WRITE:
                    // We are expecting to write data
                    //
                    // Record the amount received, but
                    // ignore the data itself (in buffer).
                    handled_data_len += bufsize;
                    INFO("Received %d bytes of data, %d received total, %d expected total", bufsize, handled_data_len, expected_data_len);
            
                    if (expected_data_len == handled_data_len) {
                        // Have received all data

                        // Send status response
                        send_status_response(STATUS_READY, handled_data_len);

                        // Reset back to waiting for a command
                        reset_data();
                        current_command = CMD_NONE;
                    }
                break;

                case CMD_READ:
                    // We are not expecting to receive data, instead we're
                    // expecting to provide it
                    INFO("Unexpectedly received data when executing READ command: %d bytes", bufsize);
                    send_status_response(STATUS_BUSY, 0);
                    break;

                default:
                    INFO("Received data while in invalid current command: 0x%02x", current_command);
                    send_status_response(STATUS_ERROR, 0);
                    break;
            }
        }
    } else {
        INFO("Received data on unexpected interface 0x%02x - ignoring", itf);
    }

    // The following code is copied from the tinyusb webusb_serial example.  
    // I think that when RX_BUFSIZE is > 0 tinyusb uses a ring buffer
    // internally to capture and supply data to us.  I suspect if we don't
    // flush the buffer, it fills up and we stop receiving data.
    //
    // Additionally, I think if RX_BUFSIZE > 0 we can either call
    // tud_vendor_read() to get the data, or get it from buffer/bufsize
    // passed into this callback.

    // if using RX buffered is enabled, we need to flush the buffer to make room for new data
#if CFG_TUD_VENDOR_RX_BUFSIZE > 0
    tud_vendor_read_flush();
#endif

    return;
}

// This callback is called once data we have sent (using tud_vendor_write())
// has actually been sent.
void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    INFO("Sent 0x%02x bytes", sent_bytes);
}

// This callback handles control transfers.
//
// stage may be one of:
// - CONTROL_STAGE_IDLE 
// - CONTROL_STAGE_SETUP
// - CONTROL_STAGE_DATA
// - CONTROL_STAGE_ACK
//
// Direction is specified by request->bmRequestType containing bit TUSB_DIR_IN
//
// tinyusb expects us to do any work associated with a control transfer in the
// setup stage, and we must send the response using tud_control_xfer().  We
// will then get called subsequently with DATA and ACK (and I'm not sure why
// the stack bothers, as we don't seem to be able to do anything useful at
// this stage).
//
// In our implementation we are only implementing CLASS requests, those
// directed at our vendor interface, and those IN (i.e. where the host wants
// us to send it data).
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
    // In our control protocol, responses can be up to 8 bytes.  This is in
    // effect, and arbitrary value. 
    static uint8_t ctrl_rsp[8];
    static uint8_t rsp_len;

    // Used to test the diretion
    bool dir_in = (request->bmRequestType_bit.direction == TUSB_DIR_IN) ? true : false; 

    INFO("Control transfer: if=0x%02x stage=%d req=0x%02x type=0x%02x dir=%s wValue=0x%04x wIndex=0x%04x wLength=%d",
        request->wIndex,
        stage,
        request->bRequest,
        request->bmRequestType_bit.type,
        dir_in ? "IN" : "OUT",
        request->wValue,
        request->wIndex,
        request->wLength);

    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_CLASS) {
        INFO("Control transfer - Ignoring unexpected type 0x%02x", request->bmRequestType);
        return false;
    }

    if (request->wIndex != ITF_NUM_VENDOR) {
        INFO("Control transfer - Ignoring unexpected interface 0x%02x", request->wIndex);
        return false;
    }

    switch (stage) {
        case CONTROL_STAGE_SETUP:
            // The supported control requests are defined in include.h.  They
            // can be considered arbitrary, although in reality they were
            // chosen to emulate another USB device (an xum1541).
            
            // Zero out the response buffer
            memset(ctrl_rsp, 0, sizeof(ctrl_rsp));

            // Handle the request
            switch (request->bRequest) {
                case CTRL_ECHO:
                    // Echo back the echo command
                    INFO("Control transfer - Echo");
                    
                    // This returns data so must be an IN request (i.e. the
                    // host will accept data from the device)
                    if (!dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    ctrl_rsp[0] = CTRL_ECHO;
                    rsp_len = 1;
                    break;

                case CTRL_INIT:
                    // Remember, our protocol is arbitary - there is no need
                    // to return data in this format or with these values in
                    // the general case
                    INFO("Control transfer - Init");

                    // This returns data so must be an IN request (i.e. the
                    // host will accept data from the device)
                    if (!dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    // Initialized our protocol handling
                    init_protocol_handling(); 

                    // Return a control response
                    memset(ctrl_rsp, 0, sizeof(ctrl_rsp));
                    ctrl_rsp[0] = 0x08;  // Firmware version
                    ctrl_rsp[1] = 0x03; // Capabilities
                    ctrl_rsp[2] = 0x0;
                    rsp_len = sizeof(ctrl_rsp);
                    break;

                case CTRL_RESET:
                    // No-op, and zero length response 
                    INFO("Control transfer - Reset");

                    // This does not return data so must be an OUT request
                    if (dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    rsp_len = 0;
                    break;

                case CTRL_SHUTDOWN:
                    // No-op, and zero length response
                    INFO("Control transfer - Shutdown");

                    // This does not return data so must be an OUT request
                    if (dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    rsp_len = 0;
                    break;

                case CTRL_ENTER_BOOTLOADER:
                    // Log, flush the log, and then reboot in DFU
                    // (programming) mode 
                    INFO("Control transfer - Enter bootloader");

                    // This does not return data so must be an OUT request
                    if (dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    fflush(stdout);
                    enter_bootloader();
                    break;

                case CTRL_GITREV:
                    // Return the git revision of this example source code.
                    // This is set up in CMkakeLists.txt
                    INFO("Control transfer - Git Revision");

                    // This returns data so must be an IN request (i.e. the
                    // host will accept data from the device)
                    if (!dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    strncpy(ctrl_rsp, __GIT_REVISION__, sizeof(ctrl_rsp));
                    rsp_len = sizeof(ctrl_rsp);
                    break;

                case CTRL_GCCVER:
                    // Return the GCC version used to compile the source.
                    // __VERSION__ is a standard GCC macro.

                    // This returns data so must be an IN request (i.e. the
                    // host will accept data from the device)
                    if (!dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    INFO("Control transfer - GCC Version");
                    strncpy(ctrl_rsp, __VERSION__, sizeof(ctrl_rsp));
                    rsp_len = sizeof(ctrl_rsp);
                    break;

                case CTRL_SDKVER:
                    // Return the Pico SDK version used to build this example.
                    // PICO_SDK_VERSION_STRING is a standard Pico SDK macro.

                    // This returns data so must be an IN request (i.e. the
                    // host will accept data from the device)
                    if (!dir_in) {
                        INFO("Unexpected direction");
                        return false;
                    }

                    INFO("Control transfer - SDK Version");
                    strncpy(ctrl_rsp, PICO_SDK_VERSION_STRING, sizeof(ctrl_rsp));
                    rsp_len = sizeof(ctrl_rsp);
                    break;

                default:
                    INFO("Control transfer - Unsupported type: 0x%02x, dir: %s",
                        request->bRequest, dir_in ? "IN" : "OUT");
                    return false;
            }

            // Call tud_control_xfer to send the response, returning its return
            // code
            return tud_control_xfer(rhport, request, ctrl_rsp, rsp_len);
            break;

        default:
            // Just return true for other stages
            return true;
    }
}

//
// Other functions
//

// Our core1 function
void core1(void) {
    while (true) {
        // Call our tight loop function, to demonstrate that core 1 is running
        example_tight_loop_contents("aux  loop");

        // Feed the watchdog
        watchdog_update();
    }

}

// This function just logs every so often so that we know the main loop hasn't
// frozen
void example_tight_loop_contents(char *loop_name) {
    static uint64_t count = 0;

    if ((count % LOG_INTERVAL_COUNT) == 0) {
        INFO("%s", loop_name);
    }
    count++;
}

// Reboot and enter bootsel (DFU, bootloader, or progamming) mode
void enter_bootloader(void) {
#ifdef PICO_ENTER_USB_BOOT_ON_EXIT
    // PICO_ENTER_USB_BOOT_ON_EXIT is set to 1 in CMakeLists.txt
    // We must stop core 1 from running, and then call reset_usb_boot()
    // in order to exit our program.  This, combined with the #define,
    // will cause the Pico to reboot and enter the bootloader.
    multicore_reset_core1();
    reset_usb_boot(0, 0);
#else
    INFO("Bootloader support not compiled in");
#endif
}