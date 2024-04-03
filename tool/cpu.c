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

    regs.rip = YAKVM_BIN_ENTRY;
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

static void yakvm_cpu_handle_ioio(struct vm *vm)
{
        struct registers regs;
        uint32_t info = vm->cpu.state->exit_info_1;
        assert(svm_ioio_is_size8(info));
        assert(svm_ioio_port(info) == YAKVM_IO_HAWK);

        /*
         * IN/OUT instruction is described at
         * "IN" on page 182 and "OUT" on page "267" at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf
         */
        assert(ioctl(vm->cpu.fd, YAKVM_GET_REGS, &regs) == 0);
        if (svm_ioio_type(info) == SVM_IOIO_TYPE_IN) {
                regs.rax = yakvm_device_io_get();
        } else {
                yakvm_device_io_set(regs.rax);
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
                        assert(ioctl(vm->vmfd, YAKVM_MMAP_PAGE,
                                        yakvm_page(vm->cpu.state->exit_info_2)) == 0);
                        break;

                case SVM_EXIT_EXCP_BASE + DB_VECTOR:
                        log(LOG_INFO, "guest executes instruction at %#x:%#lx",
                            vm->cpu.state->cs, vm->cpu.state->rip);
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
