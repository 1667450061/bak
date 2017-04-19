#! /bin/sh

make -j8
sleep 1
adb root
sleep 2
adb push ./arch/arm/boot/zImage /tmp
sleep 2
adb shell dd if=/tmp/zImage of=/dev/block/mmcblk0p10
sleep 2
adb shell sync
sleep 1
adb shell reboot
