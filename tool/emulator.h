#ifndef __YAKVM_TOOL_EMULATOR_H_

    #define __YAKVM_TOOL_EMULATOR_H_

    #include "../include/cpu.h"
    #include "cpu.h"
    struct vm {
        int vmfd;
        struct cpu cpu;
        char *memory;
    };

#endif // __YAKVM_TOOL_EMULATOR_H_
