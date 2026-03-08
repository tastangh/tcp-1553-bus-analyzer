#!/bin/bash
# Apply network capabilities for passive TCP sniffing

if [ "$EUID" -ne 0 ]; then
  echo "Please run this script with sudo:"
  echo "sudo ./scripts/setup_permissions.sh"
  exit 1
fi

BINARY_PATH="$(dirname "$0")/../bin/tcp_1553_bus_analyzer"

if [ ! -f "$BINARY_PATH" ]; then
    echo "Error: Binary not found at $BINARY_PATH"
    echo "Please build the project first."
    exit 1
fi

echo "Applying CAP_NET_RAW to $BINARY_PATH..."
if setcap cap_net_raw,cap_net_admin=eip "$BINARY_PATH"; then
    echo "Success! You can now run the analyzer without sudo."
    echo "Use: ./bin/tcp_1553_bus_analyzer"
else
    echo "Failed to set capabilities. Ensure 'setcap' is installed."
    exit 1
fi
