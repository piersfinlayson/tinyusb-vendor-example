# TinyUSB Vendor Example Protocol

This document describes the USB protocol implemented in the TinyUSB vendor example project.

## Control Transfers

The device supports the following control requests:

- `CTRL_ECHO` (0x00) - Echo test
- `CTRL_INIT` (0x01) - Initialize device
- `CTRL_RESET` (0x02) - Reset device
- `CTRL_SHUTDOWN` (0x03) - Shutdown device
- `CTRL_ENTER_BOOTLOADER` (0x04) - Enter bootloader mode
- `CTRL_GITREV` (0x06) - Get Git revision
- `CTRL_GCCVER` (0x07) - Get GCC version
- `CTRL_SDKVER` (0x08) - Get Pico SDK version

## Bulk Transfers

### Command Format
Commands are 4 bytes:
```
Byte 0: Command (READ=8, WRITE=9)
Byte 1: Protocol ID
Bytes 2-3: Data length (little-endian)
```

### Bulk Status Response Format
Status responses are 3 bytes:
```
Byte 0: Status code (BUSY=1, READY=2, ERROR=3)
Bytes 1-2: Data length (little-endian)
```

### Protocol Flow
1. Host sends command (4 bytes)
2. For WRITE commands:
   - Host sends data (if length > 0)
   - Device sends status response
3. For READ commands:
   - Device sends data (if length > 0)
   - Device does not send status response

### Example
A typical READ command requesting 256 bytes:
```
Host -> Device: [0x08, 0x10, 0x00, 0x01]  # READ command, protocol 16, 256 bytes
Device -> Host: [data bytes...]            # 256 bytes of data
Device -> Host: [0x02, 0x00, 0x01]        # STATUS_READY, 256 bytes confirmed
```