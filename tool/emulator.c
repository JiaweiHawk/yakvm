#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "emulator.h"
#include "memory.h"
#include "../include/memory.h"
#include "../include/vm.h"
#include "../include/yakvm.h"

int main(int argc, char *argv[])
{
        struct vm vm;
        int yakvmfd, ret;

        yakvmfd = open("/dev/yakvm", O_RDWR | O_CLOEXEC);
        if (yakvmfd < 0) {
                ret = errno;
                log(LOG_ERR, "open(\"/dev/yakvm\") failed with error %s",
                    strerror(errno));
        }

        vm.vmfd = ioctl(yakvmfd, YAKVM_CREATE_VM);
        if (vm.vmfd < 0) {
                ret = errno;
                log(LOG_ERR, "ioctl(YAKVM_CREATE_VM) failed with error %s",
                    strerror(errno));
                goto close_yakvmfd;
        }

        ret = yakvm_create_memory(&vm);
        if (ret) {
                log(LOG_ERR, "yakvm_create_memory() "
                    "failed with error %d", ret);
                goto close_vmfd;
        }

        vm.cpufd = ioctl(vm.vmfd, YAKVM_CREATE_VCPU);
        if (vm.cpufd < 0) {
                ret = errno;
                log(LOG_ERR, "ioctl(YAKVM_CREATE_VCPU) failed with error %s",
                    strerror(errno));
                goto destroy_memory;
        }
        assert((ioctl(vm.vmfd, YAKVM_CREATE_VCPU) == -1) && (errno == EEXIST));

        vm.cpu_state = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
                             MAP_SHARED, vm.cpufd, 0);
        if (vm.cpu_state == MAP_FAILED) {
                ret = errno;
                log(LOG_ERR, "mmap() failed with error %s",
                    strerror(errno));
                goto close_cpufd;
        }

        assert(vm.cpu_state->exitcode == YAKVM_VCPU_EXITCODE_NULL);
        assert(ioctl(vm.cpufd, YAKVM_RUN) == 0);
        assert(vm.cpu_state->exitcode == YAKVM_VCPU_EXITCODE_EXCEPTION_PF);

close_cpufd:
        close(vm.cpufd);
destroy_memory:
        yakvm_destroy_memory(&vm);
close_vmfd:
        close(vm.vmfd);
close_yakvmfd:
        close(yakvmfd);

        return ret;
}
