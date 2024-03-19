#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../include/yakvm.h"

int main(int argc, char *argv[])
{
    int yakvmfd, vmfd, ret;

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
        goto put_yakvm;
    }

    close(vmfd);
put_yakvm:
    close(yakvmfd);

    return ret;
}
