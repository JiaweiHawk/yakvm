#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../include/yakvm.h"

int main(int argc, char *argv[])
{
    int yakvmfd, vmfd, ret;

    log(LOG_INFO, "build a virtual machine based on YAKVM");

    yakvmfd = open("/dev/yakvm", O_RDWR | O_CLOEXEC);
    if (yakvmfd < 0) {
        ret = errno;
        log(LOG_ERR, "open(\"/dev/yakvm\") failed with error %s",
            strerror(errno));
    }
    log(LOG_INFO, "open the \"/dev/yakvm\" successfully");

    vmfd = ioctl(yakvmfd, YAKVM_CREATE_VM);
    if (vmfd < 0) {
        ret = errno;
        log(LOG_ERR, "ioctl(YAKVM_CREATE_VM) failed with error %s",
            strerror(errno));
        goto put_yakvm;
    }
    log(LOG_INFO, "create the vm successfully");

    close(vmfd);
put_yakvm:
    close(yakvmfd);

    return ret;
}
