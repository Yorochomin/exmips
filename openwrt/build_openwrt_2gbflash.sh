#!/bin/sh

git clone --depth 1 -b v24.10.0 https://github.com/openwrt/openwrt.git

cd openwrt

patch -p1 < ../openwrt_2gb_spiflasg.patch

# This build process is designed based on the following page
# https://openwrt.org/docs/guide-developer/toolchain/use-buildsystem
./scripts/feeds update -a
./scripts/feeds install -a
wget https://downloads.openwrt.org/releases/24.10.0/targets/ath79/tiny/config.buildinfo  -O .config
make defconfig
# make menuconfig
make -j4

cp bin/targets/ath79/tiny/*nec_wg600hp-initramfs-kernel.bin ..
cp bin/targets/ath79/tiny/*nec_wg600hp-squashfs-sysupgrade.bin ..

