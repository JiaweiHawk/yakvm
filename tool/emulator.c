#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "cpu.h"
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

        ret = yakvm_create_cpu(&vm);
        if (ret) {
                log(LOG_ERR, "yakvm_create_cpu() "
                    "failed with error %d", ret);
                goto destroy_memory;
        }

        yakvm_cpu_run(&vm);

        yakvm_destroy_cpu(&vm);
destroy_memory:
        yakvm_destroy_memory(&vm);
close_vmfd:
        close(vm.vmfd);
close_yakvmfd:
        close(yakvmfd);

        return ret;
}
