#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "memory.h"
#include "../include/memory.h"

/* load the bin to *YAKVM_BIN_ENTRY* */
static int yakvm_load_bin(char *memory, const char *bin)
{
        int fd, ret = 0;
        struct stat stat;

        fd = open(bin, O_RDONLY);
        if (fd == -1) {
                ret = errno;
                log(LOG_ERR, "open() failed with error %s",
                    strerror(ret));
                goto out;
        }

        ret = fstat(fd, &stat);
        if (ret == -1) {
                ret = errno;
                log(LOG_ERR, "fstat() failed with error %s",
                    strerror(ret));
                goto close_fd;
        }
        if (stat.st_size > YAKVM_MEMORY) {
                ret = -E2BIG;
                log(LOG_ERR, "stat.st_size %ld is out-of-bounds [1, %d]",
                    stat.st_size, YAKVM_MEMORY);
                goto close_fd;
        }

        ret = read(fd, memory + YAKVM_BIN_ENTRY, stat.st_size);
        if (ret == -1) {
                ret = errno;
                log(LOG_ERR, "read() failed with error %s",
                    strerror(ret));
                goto close_fd;
        }
        assert(ret == stat.st_size);

        ret = 0;

close_fd:
        assert(close(fd) == 0);
out:
        return ret;
}

int yakvm_create_memory(struct vm *vm, const char *bin)
{
        int ret = 0;

        vm->memory = mmap(NULL, YAKVM_MEMORY, PROT_READ | PROT_WRITE,
                         MAP_SHARED, vm->vmfd, 0);
        if (vm->memory == MAP_FAILED) {
                ret = errno;
                log(LOG_ERR, "mmap() failed with error %s",
                    strerror(errno));
                goto out;
        }

        ret = yakvm_load_bin(vm->memory, bin);
        if (ret != 0) {
                log(LOG_ERR, "yakvm_load_bin() "
                    "failed with error %d", ret);
                goto munmap;
        }

        return 0;

munmap:
        assert(!munmap(vm->memory, YAKVM_MEMORY));
out:
        return ret;
}

void yakvm_destroy_memory(struct vm *vm)
{
    assert(!munmap(vm->memory, YAKVM_MEMORY));
}
