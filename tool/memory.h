#ifndef __YAKVM_TOOL_MEMORY_H_

    #define __YAKVM_TOOL_MEMORY_H_

    #include "emulator.h"
    int yakvm_create_memory(struct vm *vm);
    void yakvm_destroy_memory(struct vm *vm);

#endif // __YAKVM_TOOL_MEMORY_H_
