#!/bin/sh

# For excuting following lines,
# some developper tools, such as bison, flex, libssl-dev, gcc-mips-linux-gnu, are necessary.

git clone --depth 1 -b devadd/ar9344_aterm https://github.com/musashino-build/u-boot.git
cd u-boot

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
