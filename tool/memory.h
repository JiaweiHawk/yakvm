#ifndef __YAKVM_TOOL_MEMORY_H_

    #define __YAKVM_TOOL_MEMORY_H_

    #include "../include/memory.h"
    /* transfer the gpa to its physical page address */
    static inline unsigned long yakvm_page(unsigned long pa)
    {
        return pa & PAGE_MASK;
    }

    /*
     * npf error code is describe in "15.25.6" on page 550 at
     * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
     */
    #define YAKVM_EXIT_NPF_INFO1_RW         (1ul << 1)
    #define YAKVM_EXIT_NPF_INFO1_US         (1ul << 2)
    #define YAKVM_EXIT_NPF_INFO1_ID         (1ul << 4)
    #define YAKVM_EXIT_NPF_INFO1_NPT        (1ul << 32)

    #include "emulator.h"
    int yakvm_create_memory(struct vm *vm);
    void yakvm_destroy_memory(struct vm *vm);

#endif // __YAKVM_TOOL_MEMORY_H_
