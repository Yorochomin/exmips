#!/bin/sh

# This version fetch U-Boot with the explicit hash.
# Usualy, the explicit hash is not necessary.

mkdir u-boot
cd u-boot
git init
git remote add origin https://github.com/musashino-build/u-boot.git
git fetch --depth 1 origin 568e0c99051c659c51a2fad723549301d6ebf0f5
git checkout FETCH_HEAD


patch -p1 < ../u-boot.patch
make nec_ar9344_aterm_defconfig
make CC=mips-linux-gnu-gcc LD=mips-linux-gnu-ld OBJCOPY=mips-linux-gnu-objcopy

# following lines add padding-0 to make the image 256KB exactly.
cp u-boot.bin u-boot.resized.bin
truncate -s 262144 u-boot.resized.bin

wget https://downloads.openwrt.org/releases/24.10.0/targets/ath79/tiny/openwrt-24.10.0-ath79-tiny-nec_wg600hp-initramfs-kernel.bin
wget https://downloads.openwrt.org/releases/24.10.0/targets/ath79/tiny/openwrt-24.10.0-ath79-tiny-nec_wg600hp-squashfs-sysupgrade.bin

cat u-boot.resized.bin openwrt-24.10.0-ath79-tiny-nec_wg600hp-initramfs-kernel.bin    > ../firm_u-boot_initramfs.bin
cat u-boot.resized.bin openwrt-24.10.0-ath79-tiny-nec_wg600hp-squashfs-sysupgrade.bin > ../firm_u-boot_squashfs.bin
