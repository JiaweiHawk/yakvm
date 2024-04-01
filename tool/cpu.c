#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "cpu.h"
#include "memory.h"
#include "emulator.h"
#include "../include/memory.h"
#include "../include/vm.h"

int yakvm_create_cpu(struct vm *vm)
{
    int ret = 0;

    vm->cpu.fd = ioctl(vm->vmfd, YAKVM_CREATE_VCPU);
    if (vm->cpu.fd < 0) {
            ret = errno;
            log(LOG_ERR, "ioctl(YAKVM_CREATE_VCPU) failed with error %s",
                strerror(errno));
            goto out;
    }

    assert((ioctl(vm->vmfd, YAKVM_CREATE_VCPU) == -1) && (errno == EEXIST));

    vm->cpu.state = mmap(NULL, sizeof(*vm->cpu.state),
                         PROT_READ | PROT_WRITE,MAP_SHARED, vm->cpu.fd, 0);
    if (vm->cpu.state == MAP_FAILED) {
            ret = errno;
            log(LOG_ERR, "mmap() failed with error %s",
                strerror(errno));
            goto close_cpufd;
    }

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
  * When the *vmrun* instruction exits(back to the host), an
  * exit/reason code is stored in the *EXITCODE* field in the
  * *vmcb* according to *Appendix C* on page 745 at
  * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
  */
void yakvm_cpu_run(struct vm *vm)
{
    assert(ioctl(vm->cpu.fd, YAKVM_RUN) == 0);
    assert(vm->cpu.state->exit_code == SVM_EXIT_NPF &&
           vm->cpu.state->exit_info_1 == (YAKVM_EXIT_NPF_INFO1_US |
                                          YAKVM_EXIT_NPF_INFO1_ID |
                                          YAKVM_EXIT_NPF_INFO1_NPT) &&
           vm->cpu.state->exit_info_2 == 0xfffff000);
    assert(ioctl(vm->vmfd, YAKVM_MMAP_PAGE, yakvm_page(vm->cpu.state->exit_info_2)) == 0);
    assert(ioctl(vm->cpu.fd, YAKVM_RUN) == 0);
    assert(vm->cpu.state->exit_code == SVM_EXIT_NPF &&
           vm->cpu.state->exit_info_1 == (YAKVM_EXIT_NPF_INFO1_RW |
                                          YAKVM_EXIT_NPF_INFO1_US |
                                          YAKVM_EXIT_NPF_INFO1_NPT) &&
           vm->cpu.state->exit_info_2 == 0x0);
}
