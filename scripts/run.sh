#!/bin/bash
# Initialization
cd `dirname $0`
SCRIPTDIR=`pwd`
cd -

cd $SCRIPTDIR/../bin/
./tcp_1553_bus_analyzer
