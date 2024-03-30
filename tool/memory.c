#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include "memory.h"
#include "../include/memory.h"

int yakvm_create_memory(struct vm *vm)
{
    vm->memory = mmap(NULL, YAKVM_MEMORY, PROT_READ | PROT_WRITE,
                     MAP_SHARED, vm->vmfd, 0);
    if (vm->memory == MAP_FAILED) {
            log(LOG_ERR, "mmap() failed with error %s",
                strerror(errno));
            return errno;
    }

    return 0;
}

void yakvm_destroy_memory(struct vm *vm)
{
    assert(!munmap(vm->memory, YAKVM_MEMORY));
}
