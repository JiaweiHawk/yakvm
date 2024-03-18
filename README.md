# Yet Another Kernel-based Virtual Machine

[![github CI](https://github.com/JiaweiHawk/yakvm/actions/workflows/main.yml/badge.svg)](https://github.com/JiaweiHawk/yakvm/actions/)

This is a simple **kernel-based virtual machine** for linux to understand **AMD-V(AMD Virtualization)** technology.

# Usage

## build the environment

Run the ```make env``` to generate the **YAKVM** environment image.

With this image, you can try **kernel-based virtual machine** in the image.

## boot the qemu

Run the ```make run``` to boot up the qemu with **yakvm.ko** in **/mnt/shares** on guest.

## debug the yakvm

Run the ```make debug``` to debug the **YAKVM** on the yakvm environment

## test the yakvm

Run the ```make test``` to run the tests on the yakvm environment

# Reference

- [pandengyang/peach](https://github.com/pandengyang/peach)
- [AMD/Secure virtual machine architecture reference manual](https://www.0x04.net/doc/amd/33047.pdf)
