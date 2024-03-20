#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../include/cpu.h"
#include "../include/vm.h"
#include "../include/yakvm.h"

int main(int argc, char *argv[])
{
        int yakvmfd, vmfd, cpufd, ret;

        yakvmfd = open("/dev/yakvm", O_RDWR | O_CLOEXEC);
        if (yakvmfd < 0) {
                ret = errno;
                log(LOG_ERR, "open(\"/dev/yakvm\") failed with error %s",
                    strerror(errno));
        }

        vmfd = ioctl(yakvmfd, YAKVM_CREATE_VM);
        if (vmfd < 0) {
                ret = errno;
                log(LOG_ERR, "ioctl(YAKVM_CREATE_VM) failed with error %s",
                    strerror(errno));
                goto close_yakvmfd;
        }

        cpufd = ioctl(vmfd, YAKVM_CREATE_VCPU);
        if (cpufd < 0) {
                ret = errno;
                log(LOG_ERR, "ioctl(YAKVM_CREATE_VCPU) failed with error %s",
                    strerror(errno));
                goto close_vmfd;
        }

        assert((ioctl(vmfd, YAKVM_CREATE_VCPU) == -1) && (errno == EEXIST));

        ioctl(cpufd, YAKVM_RUN);

        close(cpufd);
close_vmfd:
        close(vmfd);
close_yakvmfd:
        close(yakvmfd);

        return ret;
}
