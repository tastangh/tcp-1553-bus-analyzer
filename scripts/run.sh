#!/bin/bash
# Initialization
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."

echo "Applying network capabilities for passive sniffing..."
sudo "$SCRIPT_DIR/setup_permissions.sh"

echo "Starting TCP 1553 Suite..."
cd "$PROJECT_ROOT/bin/"
./tcp_1553_bus_analyzer
