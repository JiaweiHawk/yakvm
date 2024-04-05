#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "cpu.h"
#include "devices.h"
#include "memory.h"
#include "emulator.h"
#include "../include/vm.h"

int yakvm_create_cpu(struct vm *vm)
{
    int ret = 0;
    struct registers regs = {};

    vm->cpu.fd = ioctl(vm->vmfd, YAKVM_CREATE_VCPU);
    sleep(2); // for test check
    if (vm->cpu.fd < 0) {
            ret = errno;
            log(LOG_ERR, "ioctl(YAKVM_CREATE_VCPU) failed with error %s",
                strerror(errno));
            goto out;
    }

    assert((ioctl(vm->vmfd, YAKVM_CREATE_VCPU) == -1) && (errno == EEXIST));
    sleep(2); // for test check

    vm->cpu.state = mmap(NULL, sizeof(*vm->cpu.state),
                         PROT_READ | PROT_WRITE,MAP_SHARED, vm->cpu.fd, 0);
    if (vm->cpu.state == MAP_FAILED) {
            ret = errno;
            log(LOG_ERR, "mmap() failed with error %s",
                strerror(errno));
            goto close_cpufd;
    }

    /*
     * Normally within real mode, the segment base-address is formed by
     * shifting the selector value left four bits. However, immediately
     * following RESET or INIT, the segment base-address is not formed
     * by left-shifting the selector until the selector register is
     * loaded by software according to "14.1.5" on page 482 at
     * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
     */
    assert(ioctl(vm->cpu.fd, YAKVM_GET_REGS, &regs) == 0);
    assert(YAKVM_ENTRY % PAGE_SIZE == 0);
    regs.cs = YAKVM_ENTRY;
    regs.rip = 0;
    assert(YAKVM_STACK % PAGE_SIZE == 0);
    regs.ss = YAKVM_STACK;
    regs.rsp = 0x1000;
    assert(ioctl(vm->cpu.fd, YAKVM_SET_REGS, &regs) == 0);

    vm->cpu.mode = RUNNING;

    return 0;

close_cpufd:
    assert(!close(vm->cpu.fd));
out:
    return ret;
}

void yakvm_destroy_cpu(struct vm *vm)
{
    assert(!munmap(vm->cpu.state, sizeof(*vm->cpu.state)));
    assert(!close(vm->cpu.fd));
}

/*
 * Emulating the mmio instruction. For simplicity, we only
 * support the *movb (edx), al* and *movb al, (edx)*
 * instructions, where %edx holds the mmio address and
 * %al holds the value.
 */
static void yakvm_vcpu_handle_mmio(struct vm *vm)
{
        uint32_t ip = vm->cpu.state->cs + vm->cpu.state->rip;
        struct registers regs;

        /*
         * The address-size override prefix can override the
         * default 16-bit addresses to effective 32-bit addresses
         * according to "1.2.3" on page 9 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf
         */
        assert(vm->memory[ip] == 0x67);

        /*
         * Opcode 0x88 represents *mov mem8, reg8*, which is mmio out
         * instruction, and Opcode 0x8a represents *mov reg8, mem8*,
         * which is mmio in instruction, according to "MOV" on page 234 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf
         *
         * Operands should be in ModRM byte format, meaning
         * %edx contains the mmio address an %al holds the
         * vlaue described in "1.4.3" on page 21 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf
         */
        assert(ioctl(vm->cpu.fd, YAKVM_GET_REGS, &regs) == 0);
        switch (vm->memory[ip + 1]) {
                case 0x88:
                        assert(vm->memory[ip + 2] == 0x02);
                        assert(regs.rdx == YAKVM_MMIO_HAWK);
                        yakvm_device_mmio_set(regs.rax);
                        break;

                case 0x8a:
                        assert(vm->memory[ip + 2] == 0x02);
                        assert(regs.rdx == YAKVM_MMIO_HAWK);
                        regs.rax = yakvm_device_mmio_get();
                        break;

                default:
                        /* code should not reach here */
                        assert(false);
                        break;
        }

        /* update the rip to the next instruction address */
        regs.rip += 3;
        assert(ioctl(vm->cpu.fd, YAKVM_SET_REGS, &regs) == 0);
}

static void yakvm_cpu_handle_npf(struct vm *vm)
{
        if (vm->cpu.state->exit_info_2 == YAKVM_MMIO_HAWK) {
                yakvm_vcpu_handle_mmio(vm);
        } else {
                assert(ioctl(vm->vmfd, YAKVM_MMAP_PAGE,
                             yakvm_page(vm->cpu.state->exit_info_2)) == 0);
        }
}

static void yakvm_cpu_handle_ioio(struct vm *vm)
{
        struct registers regs;
        uint32_t info = vm->cpu.state->exit_info_1;
        assert(svm_ioio_is_size8(info));
        assert(svm_ioio_port(info) == YAKVM_PIO_HAWK);

        /*
         * IN/OUT instruction is described at
         * "IN" on page 182 and "OUT" on page "267" at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf
         */
        assert(ioctl(vm->cpu.fd, YAKVM_GET_REGS, &regs) == 0);
        if (svm_ioio_type(info) == SVM_IOIO_TYPE_IN) {
                regs.rax = yakvm_device_pio_get();
        } else {
                yakvm_device_pio_set(regs.rax);
        }

        /*
         * The rip of the instruction following the IN/OUT is saved in
         * EXITINFO2, so that the VMM can easily resume the guest after
         * I/O emulation according to "15.10.2" on page 516 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        regs.rip = vm->cpu.state->exit_info_2;
        assert(ioctl(vm->cpu.fd, YAKVM_SET_REGS, &regs) == 0);
}

static int yakvm_cpu_handle_exit(struct vm *vm)
{
        switch (vm->cpu.state->exit_code) {
                case SVM_EXIT_NPF:
                        yakvm_cpu_handle_npf(vm);
                        break;

                case SVM_EXIT_EXCP_BASE + DB_VECTOR:
                        log(LOG_INFO, "guest executes instruction at "
                            "%#lx, opcode = %hhx", vm->cpu.state->cs +
                                                   vm->cpu.state->rip,
                            vm->memory[vm->cpu.state->cs +
                                       vm->cpu.state->rip]);
                        break;

                case SVM_EXIT_HLT:
                        vm->cpu.mode = HLT;
                        break;

                case SVM_EXIT_IOIO:
                        yakvm_cpu_handle_ioio(vm);
                        break;

                default:
                        log(LOG_ERR, "improper exit_code %#x",
                            vm->cpu.state->exit_code);
                        return -EINVAL;
        }

        return 0;
}

/*
 * When the *vmrun* instruction exits(back to the host), an
 * exit/reason code is stored in the *EXITCODE* field in the
 * *vmcb* according to *Appendix C* on page 745 at
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */
void yakvm_cpu_run(struct vm *vm)
{
        while(vm->cpu.mode == RUNNING) {
                assert(ioctl(vm->cpu.fd, YAKVM_RUN) == 0);
                assert(yakvm_cpu_handle_exit(vm) == 0);
        }
}
