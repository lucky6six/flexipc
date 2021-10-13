#!/bin/bash
HOME=../../
BUILD_PATH=${HOME}/build

sudo aarch64-linux-gnu-objcopy ${BUILD_PATH}/kernel.img -O binary ./rootfs/boot/kernel.bin
#img_sz=$(stat -c "%s" rootfs/boot/kernel.bin)
#img_sz=$((img_sz/1024/1024 + 1))
#sudo dd if=/dev/zero of=rootfs.img bs=1M count=$img_sz conv=sparse
sudo dd if=/dev/zero of=rootfs.img bs=1M count=16 conv=sparse
sudo mkfs.ext4 -L rootfs rootfs.img
sudo mkdir mnt
sudo mount rootfs.img mnt
sudo cp -r rootfs/* mnt
sudo umount mnt
sudo rm -r mnt
