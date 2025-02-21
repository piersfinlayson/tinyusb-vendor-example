#!/bin/bash

# Check if script is run as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

# Get the actual user who invoked sudo
ACTUAL_USER=${SUDO_USER:-$USER}
if [ "$ACTUAL_USER" = "root" ]; then
    echo "Please run this script with sudo rather than as root directly"
    exit 1
fi

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Check if rules file exists
RULES_FILE="$PROJECT_ROOT/rules/45-tinyusb-vendor-example.rules"
if [ ! -f "$RULES_FILE" ]; then
    echo "Error: Rules file not found at $RULES_FILE"
    exit 1
fi

# Check if plugdev group exists
if ! getent group plugdev > /dev/null; then
    echo "Creating plugdev group..."
    groupadd plugdev
fi

# Check if user is in plugdev group
if ! groups "$ACTUAL_USER" | grep -q "plugdev"; then
    echo "Adding user $ACTUAL_USER to plugdev group..."
    usermod -a -G plugdev "$ACTUAL_USER"
    echo "Note: You will need to log out and back in for the group changes to take effect"
fi

# Copy rules file to udev rules directory
echo "Installing udev rules..."
cp "$RULES_FILE" /etc/udev/rules.d/

# Set correct permissions
chmod 644 /etc/udev/rules.d/45-tinyusb-vendor-example.rules

# Reload udev rules
echo "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger

echo "Installation complete!"
echo "If you were added to the plugdev group, please log out and back in for changes to take effect."
echo "You may need to replug your device for the rules to take effect."