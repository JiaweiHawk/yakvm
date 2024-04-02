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

## cpu virtualization

```
 ┌───────────────────┐             ┌───────────────────┐
 │                   │             │                   │
 │ ┌──────────────┐  │             │ ┌──────────────┐  │
 │ │  hsave area  │  │             │ │  hsave area  │  │
 │ └──────────────┘  │             │ └──────────────┘  │
 │                   │             │                   │
 │ ┌───────────────┐ │             │ ┌───────────────┐ │
 │ │Virtual Machine│ │vmrun   vmrun│ │Virtual Machine│ │
 │ │ Control Block ◄─┼─────┐  ┌────┼─► Control Block │ │
 │ │  struct vmcb  │ │     │  │    │ │  struct vmcb  │ │
 │ └───────────────┘ │     │  │    │ └───────────────┘ │
 │    virtual cpu    │     │  │    │    virtual cpu    │
 └─────────┬─────────┘     │  │    └─────────┬─────────┘
           │               │  │              │
           │               │  │              │
           │               │  │              │
           │               │  │              │
           │vmexit        ┌┴──┴┐       vmexit│
           └──────────────►host◄─────────────┘
                          └────┘
```

host must complete the following steps to execute the virtual machine:
- enable svm as [yakvm_cpu_svm_enable()](./driver/main.c)
- allocate the **vmcb** and initialize its control area for intercepting and state-save area for guest state saving as [yakvm_vcpu_init_vmcb()](./driver/cpu.c)
- reserve the **hsave** area as [yakvm_create_vcpu()](./driver/main.c) and record its address in **VM_HSAVE_PA** msr as [yakvm_vcpu_run()](./driver/cpu.c) to save host state
- execute the `clgi; vmload; vmrun; vmsave; stgi` to perform the atomic state switch as [_yakvm_vcpu_run()](./driver/vcpu_run.S)

## memory virtualization

```
┌────────────────────────────────────────────────────────────────────────────┐
│                                                                            │
│  ┌─────────────────────────────────┐                                       │
│  │                                 │                                       │
│  │     ┌──────────────────────┐    │        ┌────────────────────────────┐ │
│  │     │guest physical address├────┼────▲──►│host physical address       │ │
│  │     └──────────────────────┘    │    │   └────────────────────────────┘ │
│  │                                 │    │                                  │
│  │ ┌─────────────────────────────┐ │    │                                  │
│  │ │Virtual Machine Control Block│ │  ┌─┴────────────────┐                 │
│  │ │        struct vmcb          ├─┼─►│nested page table │                 │
│  │ └─────────────────────────────┘ │  └──────────────────┘                 │
│  │            virtual cpu          │                                       │
│  └─────────────────────────────────┘                                       │
│                                                                            │
│                                      host                                  │
└────────────────────────────────────────────────────────────────────────────┘
```

host must complete the following steps to supported memory virtualization
- enable **NP_ENABLE** bit in the **vmcb** as [yakvm_vcpu_init_vmcb()](./driver/cpu.c)
- create the **nested page table** entry to map **gpa** with **hpa** as [yakvm_vm_ioctl_mmap_page()](./driver/vm.c)

## device virtualization

Considering there are two ways to access devices under x86 architecture: **Port I/O(PIO)** and **Memory Mapped I/O(MMIO)**, we should virtualizing there two methods.

### PIO

PIO virtualization can be achieved by configuring the **vmcb** to intercept PIO.

### MMIO

For MMIO, remove the **_PAGE_PRESENT** flag from the corresponding MMIO memory's **Nested Paging Table** entry to intercept MMIO.

# Reference

- [pandengyang/peach](https://github.com/pandengyang/peach)
- [AMD64 Technology/AMD64 Architecture Programmer’s Manual Volume 2: System Programming](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf)
- [david942j/kvm-kernel-example](https://github.com/david942j/kvm-kernel-example)
- [yifengyou/learn-kvm](https://github.com/yifengyou/learn-kvm/blob/master/docs/虚拟化实现技术/虚拟化实现技术.md#amd虚拟化)
- [eliaskousk/vmrun](https://github.com/eliaskousk/vmrun/tree/dev)
- [Martin Radev/mini-svm](http://varko.xyz/mini-svm-chapter-1.html)
