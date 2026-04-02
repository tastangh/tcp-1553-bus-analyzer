#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_PATH="$SCRIPT_DIR/../bin/tcp_1553_bus_analyzer"

if [[ ! -f "$BINARY_PATH" ]]; then
  echo "Binary not found: $BINARY_PATH"
  echo "Build first: cmake -S . -B build && cmake --build build -j"
  exit 1
fi

echo "Applying capabilities for passive sniffing..."
setcap cap_net_raw,cap_net_admin=eip "$BINARY_PATH"
getcap "$BINARY_PATH" || true
echo "Permissions configured."
