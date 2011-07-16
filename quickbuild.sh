#!/bin/bash
CCTC="/home/shankar/AndroidDir/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-" 
make -j32 ARCH=arm CROSS_COMPILE=$CCTC > /dev/null
