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

# Design

## VMM interface

```
                  ┌────────┐   fd
                  │emulator├────────┬───┬───┬─────────┬──┬───┐
                  └────▲───┘        │   │   │         │  │   │
                       │            │   │   │         │  │   │
                     fd│            │   │   │         │  │   │
user space             │            │   │   │         │  │   │
───────────────────────┼────────────┼───┼───┼─────────┼──┼───┤
kernel space           │            │   │   │         │  │   │
                       │            │   │   │         │  │   │
                  ┌────▼─────┐  ┌───┼───▼───┼───┐     │  │   │
                  │/dev/yakvm│  │   │       │   │     │  │   │
                  └────▲─────┘  │ ┌─▼──┐ ┌──▼─┐ │     │  │   │
                       │        │ │vcpu│ │vcpu│ │     │  │   │
                       │        │ └────┘ └────┘ │ ┌───┼──▼───┼────┐
                       │        │virtual machine│ │   │      │    │
                       │        └──────▲────────┘ │ ┌─▼──┐ ┌─▼──┐ │
                       │               │          │ │vcpu│ │vcpu│ │
                       │               │          │ └────┘ └────┘ │
                 ┌─────▼──────┐        │          │virtual machine│
                 │yakvm driver│        │          └──────▲────────┘
                 └─────┬──────┘        │                 │
                       │               │                 │
                       └───────────────┴─────────────────┘
```

# Reference

- [pandengyang/peach](https://github.com/pandengyang/peach)
- [AMD64 Technology/AMD64 Architecture Programmer’s Manual Volume 2: System Programming](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf)
- [david942j/kvm-kernel-example](https://github.com/david942j/kvm-kernel-example)
- [yifengyou/learn-kvm](https://github.com/yifengyou/learn-kvm/blob/master/docs/虚拟化实现技术/虚拟化实现技术.md#amd虚拟化)
- [eliaskousk/vmrun](https://github.com/eliaskousk/vmrun/tree/dev)
- [Martin Radev/mini-svm](http://varko.xyz/mini-svm-chapter-1.html)
