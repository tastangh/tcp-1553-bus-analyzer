#!/bin/bash
# Initialization
cd `dirname $0`
SCRIPTDIR=`pwd`
cd - > /dev/null

echo "Applying network capabilities for passive sniffing..."
sudo setcap cap_net_raw,cap_net_admin=eip $SCRIPTDIR/../bin/tcp_1553_bus_analyzer

echo "Starting TCP 1553 Suite..."
cd $SCRIPTDIR/../bin/
./tcp_1553_bus_analyzer
