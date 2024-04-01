#ifndef __YAKVM_TOOL_CPU_H

    #define __YAKVM_TOOL_CPU_H

    #include "../include/cpu.h"
    struct cpu {
        struct state *state;
        int fd;
    };

    struct vm;
    int yakvm_create_cpu(struct vm *vm);
    void yakvm_destroy_cpu(struct vm *vm);
    void yakvm_cpu_run(struct vm *vm);

#endif // __YAKVM_CPU_H_