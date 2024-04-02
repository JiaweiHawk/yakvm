#!/usr/bin/python3
# -*- coding:utf-8 -*-
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

    INVALID_EIP = (1 << 64) - 1
    HLT_CODE = "\xf4"

    def __single_step(uc:unicorn.Uc, addr:int, size:int, vm):
        if (not addr == vm.entry):
                vm.qemu.runtil("guest executes instruction at 0:%s"%(hex(addr)),
                            timeout=args.timeout)

        if (uc.mem_read(addr, len(VM.HLT_CODE)) == VM.HLT_CODE):
            uc.emu_stop()

    def __init__(self, args:argparse.Namespace, qemu:Qemu) -> None:
        self.entry = args.entry

        # initialize vm in x86-16bit mode
        self.unicorn = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_16)

        # map vm memory
        self.unicorn.mem_map(0, args.memory)

        # set registers
        self.unicorn.reg_write(unicorn.x86_const.UC_X86_REG_SP, 0)
        self.unicorn.reg_write(unicorn.x86_const.UC_X86_REG_IP, args.entry)

        with open(args.bin, "rb") as bin:
                self.unicorn.mem_write(0, bin.read(args.memory))

        # trace all instructions
        self.unicorn.hook_add(unicorn.UC_HOOK_CODE, VM.__single_step, self)

        self.qemu = qemu

    def check(self):
        self.unicorn.emu_start(self.entry, self.INVALID_EIP)

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
                        help="entry address for guest bin")
    parser.add_argument("--memory", action="store",
                        type=int, default=2 * 1024 * 1024,
                        help="guest memory size")
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
