#ifndef __YAKVM_TOOL_EMULATOR_H_

    #define __YAKVM_TOOL_EMULATOR_H_

    #include "../include/cpu.h"
    struct vm {
        int vmfd, cpufd;
        struct vcpu_state *cpu_state;
        char *memory;
    };

#endif // __YAKVM_TOOL_EMULATOR_H_
