#!/usr/bin/python3
# -*- coding:utf-8 -*-
import argparse
import os
import select
import subprocess
import sys
import traceback
import time

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
    parser.add_argument("--timeout", action="store",
                        type=int, default=10,
                        help="max timeout for receiving from guest")
    args = parser.parse_args()

    try:
        # boot up the Qemu
        qemu = Qemu(command=args.command, history=args.history)

        qemu.runtil("login:", timeout=args.timeout)
        qemu.write("root\n")

        qemu.execute("insmod /mnt/shares/yakvm.ko")
        qemu.runtil("initialize yakvm", timeout=args.timeout)

        qemu.execute("ls /dev/yakvm | wc -l")
        qemu.runtil("1", timeout=args.timeout)

        qemu.execute("/mnt/shares/emulator")
        qemu.runtil("build a virtual machine based on YAKVM", timeout=args.timeout)

        qemu.execute("rmmod yakvm")
        qemu.runtil("cleanup yakvm", timeout=args.timeout)

    except:
        traceback.print_exc()
        ret = -1
    finally:
        if (qemu != None):
            qemu.kill()

    sys.exit(ret)
