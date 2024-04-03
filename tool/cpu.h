#ifndef __YAKVM_TOOL_CPU_H

        #define __YAKVM_TOOL_CPU_H

        enum mode {
                RUNNING = 0,
                HLT,
        };

        #include "../include/cpu.h"
        struct cpu {
                struct state *state;
                enum mode mode;
                int fd;
        };


        /*
         * I/O intercept information is described
         * at "15.10.2" on page 516 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        #define SVM_IOIO_TYPE_OUT       0
        #define SVM_IOIO_TYPE_IN        1
        #define SVM_IOIO_TYPE_MASK      1
        #define SVM_IOIO_TYPE_SHIFT     0
        static inline uint8_t svm_ioio_type(uint32_t info)
        {
                return (info >> SVM_IOIO_TYPE_SHIFT) & SVM_IOIO_TYPE_MASK;
        }
        #define SVM_IOIO_SZ8_MASK       1
        #define SVM_IOIO_SZ8_SHIFT      4
        #include <stdbool.h>
        static inline bool svm_ioio_is_size8(uint32_t info)
        {
                return (info >> SVM_IOIO_SZ8_SHIFT) & SVM_IOIO_SZ8_MASK;
        }
        #define SVM_IOIO_PORT_MASK      ((1ul << 16) - 1)
        #define SVM_IOIO_PORT_SHIFT     16
        static inline uint16_t svm_ioio_port(uint32_t info)
        {
                return (info >> SVM_IOIO_PORT_SHIFT) & SVM_IOIO_PORT_MASK;
        }

        struct vm;
        int yakvm_create_cpu(struct vm *vm);
        void yakvm_destroy_cpu(struct vm *vm);
        void yakvm_cpu_run(struct vm *vm);

#endif // __YAKVM_CPU_H_
