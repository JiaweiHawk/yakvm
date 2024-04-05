#!/usr/bin/python3
# -*- coding:utf-8 -*-
from __future__ import annotations
import argparse
import os
import select
import subprocess
import sys
import traceback
import time
import unicorn
import typing

class QemuTerminate(Exception):
    pass

class Qemu:

    def __init__(self, command:str, history:str):

        self.proc = subprocess.Popen("exec " + command, shell=True,
                                     stdout=subprocess.PIPE,
                                     stdin=subprocess.PIPE)
        self.output = ""
        self.outbytes = bytearray()


        self.history = history

        with open(history, "w") as f:
            f.truncate()
            f.write("#!/bin/bash\n")

    def _read(self) -> None:
        rset, _, _ = select.select([self.proc.stdout.fileno()], [], [], 1)

        if (rset == []):
            self.proc.poll()
            if (self.proc.returncode != None):
                raise QemuTerminate
            return

        self.outbytes += os.read(self.proc.stdout.fileno(), 4096)
        res = self.outbytes.decode("utf-8", "ignore")
        self.output += res
        self.outbytes = self.outbytes[len(res.encode("utf-8")):]

    def runtil(self, string:str, timeout:int) -> None:
        cursor = 0
        cur = time.time()
        while(True):
            self._read()
            if (string in self.output):
                break
            print(self.output[cursor:], end="", flush=True)
            if (time.time() - cur > timeout):
                raise TimeoutError
            cursor = len(self.output)

        idx = self.output.find(string) + len(string)
        print(self.output[cursor:idx], end="", flush=True)
        self.output = self.output[idx:]

    def write(self, buf:str) -> None:
        buf:bytes = buf.encode("utf-8")
        self.proc.stdin.write(buf)
        self.proc.stdin.flush()

    def execute(self, command:str) -> None:
        with open(self.history, "a") as f:
            f.write(command + "\n")

        self.runtil(":~#", timeout=60)
        self.write(command + "\n")

    def kill(self) -> None:
        self.proc.kill()
        self.proc.wait()

class VM:

    UNREACHABLE_EIP = (1 << 64) - 1
    HLT_CODE = bytearray(b"\xf4")
    IN_CODE = bytearray(b"\xec")
    OUT_CODE = bytearray(b"\xee")
    MMIO_READ_CODE = bytearray(b"\x67\x8a\x02")
    MMIO_WRITE_CODE = bytearray(b"\x67\x88\x02")

    def __emulate_mmio(uc:unicorn.Uc, access:int, addr:int, size:int, value:int, vm:VM):
        assert(addr == vm.mmio_hawk_addr)
        assert(size == 1)
        ip = uc.reg_read(unicorn.x86_const.UC_X86_REG_IP)

        uc.emu_stop()
        if (access == unicorn.UC_MEM_WRITE_UNMAPPED):
            assert(uc.mem_read(ip, len(VM.MMIO_WRITE_CODE)) == VM.MMIO_WRITE_CODE)
            vm.mmio_hawk_val = uc.reg_read(unicorn.x86_const.UC_X86_REG_AL)
        elif (access == unicorn.UC_MEM_READ_UNMAPPED):
            assert(uc.mem_read(ip, len(VM.MMIO_READ_CODE)) == VM.MMIO_READ_CODE)
            uc.reg_write(unicorn.x86_const.UC_X86_REG_AL, vm.mmio_hawk_val - 1)
        else:
            assert(False)
        uc.emu_start(ip + 3, VM.UNREACHABLE_EIP, timeout=10 * 1000)

    def __single_step(uc:unicorn.Uc, addr:int, size:int, vm:VM):
        if (not addr == vm.entry):
                vm.qemu.runtil("guest executes instruction at %s, "
                               "opcode = %s"%(hex(addr),
                               hex(uc.mem_read(addr, 1)[0])[2:]),
                               timeout=args.timeout)
        if (uc.mem_read(addr, size) == VM.HLT_CODE):
            uc.emu_stop()
        elif (uc.mem_read(addr, size) == VM.OUT_CODE):
            assert(uc.reg_read(unicorn.x86_const.UC_X86_REG_DL) == vm.pio_hawk_port)
            uc.emu_stop()
            vm.pio_hawk_val = uc.reg_read(unicorn.x86_const.UC_X86_REG_AL)
            uc.emu_start(addr + size, VM.UNREACHABLE_EIP, timeout=10 * 1000)
        elif (uc.mem_read(addr, size) == VM.IN_CODE):
            assert(uc.reg_read(unicorn.x86_const.UC_X86_REG_DL) == vm.pio_hawk_port)
            uc.emu_stop()
            uc.reg_write(unicorn.x86_const.UC_X86_REG_AL, 2)
            uc.emu_start(addr + size, VM.UNREACHABLE_EIP, timeout=10 * 1000)

    def __init__(self, args:argparse.Namespace, qemu:Qemu) -> None:
        self.entry = args.entry

        # initialize vm in x86-16bit mode
        self.unicorn = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_16)

        # map vm memory
        assert(args.mmio % 4096 == 0)
        self.unicorn.mem_map(args.mmio + 4096, args.memory)

        # set registers
        self.unicorn.reg_write(unicorn.x86_const.UC_X86_REG_SS, args.stack // 0x10)
        self.unicorn.reg_write(unicorn.x86_const.UC_X86_REG_SP, 0x1000)

        self.pio_hawk_port = args.pio
        self.pio_hawk_val = None
        self.mmio_hawk_addr = args.mmio
        self.mmio_hawk_val = None

        with open(args.bin, "rb") as bin:
            assert((args.mmio / 4096) < (args.entry / 4096))
            self.unicorn.mem_write(args.entry, bin.read(args.memory))

        # trace all instructions
        self.unicorn.hook_add(unicorn.UC_HOOK_CODE, VM.__single_step, self)
        # trace mmio instructions
        self.unicorn.hook_add(unicorn.UC_HOOK_MEM_READ_UNMAPPED | unicorn.UC_HOOK_MEM_WRITE_UNMAPPED,
                              VM.__emulate_mmio, self)

        self.qemu = qemu

    def check(self):
        # emulate for the 10 seconds
        self.unicorn.emu_start(self.entry, VM.UNREACHABLE_EIP, timeout=10 * 1000)

if __name__ == "__main__":
    ret = 0
    qemu:Qemu = None

    parser = argparse.ArgumentParser(description="yaf test script")
    parser.add_argument("--command", action="store",
                        type=str, required=True,
                        help="command to boot up qemu")
    parser.add_argument("--history", action="store",
                        type=str, required=True,
                        help="path to store executed commands")
    parser.add_argument("--bin", action="store",
                        type=str, required=True,
                        help="guest binary path")
    parser.add_argument("--timeout", action="store",
                        type=int, default=10,
                        help="max timeout for receiving from guest")
    parser.add_argument("--entry", action="store",
                        type=int, default=0,
                        help="vm entry address")
    parser.add_argument("--stack", action="store",
                        type=int, default=0,
                        help="vm stack address")
    parser.add_argument("--memory", action="store",
                        type=int, default=2 * 1024 * 1024,
                        help="guest memory size")
    parser.add_argument("--pio", action="store",
                        type=int, default=0,
                        help="port for device PIO_HAWK")
    parser.add_argument("--mmio", action="store",
                        type=int, default=0,
                        help="memory address for device MMIO_HAWK")
    args = parser.parse_args()

    try:
        # boot up the Qemu
        qemu = Qemu(command=args.command, history=args.history)

        qemu.runtil("login:", timeout=args.timeout)
        qemu.write("root\n")

        qemu.execute("insmod /mnt/shares/yakvm.ko")
        qemu.runtil("initialize yakvm", timeout=args.timeout)

        qemu.execute("/mnt/shares/emulator /mnt/shares/guest.bin")
        qemu.runtil("yakvm_create_vm() creates the kvm", timeout=args.timeout)
        qemu.runtil("vcpu has been created for kvm", timeout=args.timeout)

        vm:VM = VM(args, qemu)
        vm.check()

        qemu.runtil("yakvm_destroy_vm() destroys the kvm kvm-", timeout=args.timeout)

        qemu.execute("rmmod yakvm")
        qemu.runtil("cleanup yakvm", timeout=args.timeout)

    except:
        traceback.print_exc()
        ret = -1
    finally:
        if (qemu != None):
            qemu.kill()

    sys.exit(ret)
