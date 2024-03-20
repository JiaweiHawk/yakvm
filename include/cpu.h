#ifndef __YAKVM_CPU_H_

        #define __YAKVM_CPU_H_

        #ifdef __KERNEL__

        /*
         * The control area of the VMCB containing various control
         * bits including the intercept vector accroding to
         * "Appendix C" on page 93 at https://www.0x04.net/doc/amd/33047.pdf
         */

        /*
         * 32-bit intercept words in the VMCB Control Area, starting
         * at Byte offset 000h.
         */
        enum intercept_words {
                INTERCEPT_CR = 0,
                INTERCEPT_DR,
                INTERCEPT_EXCEPTION,
                INTERCEPT_WORD3,
                INTERCEPT_WORD4,
                MAX_INTERCEPT,
        };
        enum {
                /* Byte offset 000h (word 0) */
                INTERCEPT_CR0_READ = 0,
                INTERCEPT_CR3_READ = 3,
                INTERCEPT_CR4_READ = 4,
                INTERCEPT_CR8_READ = 8,
                INTERCEPT_CR0_WRITE = 16,
                INTERCEPT_CR3_WRITE = 16 + 3,
                INTERCEPT_CR4_WRITE = 16 + 4,
                INTERCEPT_CR8_WRITE = 16 + 8,
                /* Byte offset 004h (word 1) */
                INTERCEPT_DR0_READ = 32,
                INTERCEPT_DR1_READ,
                INTERCEPT_DR2_READ,
                INTERCEPT_DR3_READ,
                INTERCEPT_DR4_READ,
                INTERCEPT_DR5_READ,
                INTERCEPT_DR6_READ,
                INTERCEPT_DR7_READ,
                INTERCEPT_DR0_WRITE = 48,
                INTERCEPT_DR1_WRITE,
                INTERCEPT_DR2_WRITE,
                INTERCEPT_DR3_WRITE,
                INTERCEPT_DR4_WRITE,
                INTERCEPT_DR5_WRITE,
                INTERCEPT_DR6_WRITE,
                INTERCEPT_DR7_WRITE,
                /* Byte offset 008h (word 2) */
                INTERCEPT_EXCEPTION_OFFSET = 64,
                /* Byte offset 00Ch (word 3) */
                INTERCEPT_INTR = 96,
                INTERCEPT_NMI,
                INTERCEPT_SMI,
                INTERCEPT_INIT,
                INTERCEPT_VINTR,
                INTERCEPT_SELECTIVE_CR0,
                INTERCEPT_STORE_IDTR,
                INTERCEPT_STORE_GDTR,
                INTERCEPT_STORE_LDTR,
                INTERCEPT_STORE_TR,
                INTERCEPT_LOAD_IDTR,
                INTERCEPT_LOAD_GDTR,
                INTERCEPT_LOAD_LDTR,
                INTERCEPT_LOAD_TR,
                INTERCEPT_RDTSC,
                INTERCEPT_RDPMC,
                INTERCEPT_PUSHF,
                INTERCEPT_POPF,
                INTERCEPT_CPUID,
                INTERCEPT_RSM,
                INTERCEPT_IRET,
                INTERCEPT_INTn,
                INTERCEPT_INVD,
                INTERCEPT_PAUSE,
                INTERCEPT_HLT,
                INTERCEPT_INVLPG,
                INTERCEPT_INVLPGA,
                INTERCEPT_IOIO_PROT,
                INTERCEPT_MSR_PROT,
                INTERCEPT_TASK_SWITCH,
                INTERCEPT_FERR_FREEZE,
                INTERCEPT_SHUTDOWN,
                /* Byte offset 010h (word 4) */
                INTERCEPT_VMRUN = 128,
                INTERCEPT_VMMCALL,
                INTERCEPT_VMLOAD,
                INTERCEPT_VMSAVE,
                INTERCEPT_STGI,
                INTERCEPT_CLGI,
                INTERCEPT_SKINIT,
                INTERCEPT_RDTSCP,
                INTERCEPT_ICEBP,
                INTERCEPT_WBINVD,
        };

        struct __attribute__ ((__packed__)) vmcb_control_area {
                uint32_t intercepts[MAX_INTERCEPT];
                uint32_t reserved_1[16 - MAX_INTERCEPT];
                uint64_t iopm_base_pa;
                uint64_t msrpm_base_pa;
                uint64_t tsc_offset;
                uint32_t asid;
                uint8_t  tlb_ctl;
                uint8_t  reserved_2[3];
                uint32_t int_ctl;
                uint32_t int_vector;
                uint32_t int_state;
                uint8_t reserved_3[4];
                uint32_t exit_code;
                uint32_t exit_code_hi;
                uint64_t exit_info_1;
                uint64_t exit_info_2;
                uint32_t exit_int_info;
                uint32_t exit_int_info_err;
                uint8_t  np_enable;
                uint8_t  reserved_4[23];
                uint32_t event_inj;
                uint32_t event_inj_err;
                uint64_t nested_cr3;
                uint64_t virt_ext;
                uint8_t  reserved_5[832];
        } __packed;
        static_assert(sizeof(struct vmcb_control_area) == 1024);

        /*
         * The state-save area of the VMCB containing saved guest state
         * accroding to "Appendix C" on page 99 at
         * https://www.0x04.net/doc/amd/33047.pdf
         */
        struct __attribute__ ((__packed__)) vmcb_seg {
                uint16_t selector;
                uint16_t attrib;
                uint32_t limit;
                uint64_t base;
        };

        /* Save area definition for legacy and SEV-MEM guests */
        struct  __attribute__ ((__packed__)) vmcb_save_area {
                struct vmcb_seg es;
                struct vmcb_seg cs;
                struct vmcb_seg ss;
                struct vmcb_seg ds;
                struct vmcb_seg fs;
                struct vmcb_seg gs;
                struct vmcb_seg gdtr;
                struct vmcb_seg ldtr;
                struct vmcb_seg idtr;
                struct vmcb_seg tr;
                /* Reserved fields are named following their struct offset */
                uint8_t         reserved_0xa0[43];
                uint8_t         cpl;
                uint8_t         reserved_0xcc[4];
                uint64_t        efer;
                uint64_t        reserved_0xd8[112];
                uint64_t        cr4;
                uint64_t        cr3;
                uint64_t        cr0;
                uint64_t        dr7;
                uint64_t        dr6;
                uint64_t        rflags;
                uint64_t        rip;
                uint64_t        reserved_0x180[88];
                uint64_t        rsp;
                uint8_t         reserved_0x1e0[24];
                uint64_t        rax;
                uint64_t        star;
                uint64_t        lstar;
                uint64_t        cstar;
                uint64_t        sfmask;
                uint64_t        kernel_gs_base;
                uint64_t        sysenter_cs;
                uint64_t        sysenter_esp;
                uint64_t        sysenter_eip;
                uint64_t        cr2;
                uint8_t         reserved_0x248[32];
                uint64_t        g_pat;
                uint64_t        dbgctl;
                uint64_t        br_from;
                uint64_t        br_to;
                uint64_t        last_excp_from;
                uint64_t        last_excp_to;
        };

        struct __attribute__ ((__packed__)) vmcb {
                struct vmcb_control_area control;
                struct vmcb_save_area save;
        };
        static_assert(sizeof(struct vmcb) <= 4096);


        #include <linux/mutex.h>
        #include "vm.h"
        /* virtual cpu data structure */
        struct vcpu {
                struct vmcb *vmcb;
                struct vm *vm;
        };

        /* create the vcpu */
        extern struct vcpu* yakvm_create_vcpu(struct vm *vm);

        /* destory the vcpu */
        extern void yakvm_destroy_vcpu(struct vcpu *vcpu);

        #include <linux/fs.h>
        extern const struct file_operations yakvm_vcpu_fops;

    #endif // __KERNEL__

#endif // __YAKVM_CPU_H_
