#ifndef __YAKVM_CPU_H_

        #define __YAKVM_CPU_H_

        #ifdef __KERNEL__

                /*
                 * The control area of the VMCB containing various control
                 * bits including the intercept vector accroding to
                 * "Appendix B" on page 730 at
                 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
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
                        INTERCEPT_WORD5,
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
                        INTERCEPT_MONITOR,
                        INTERCEPT_MWAIT,
                        INTERCEPT_MWAIT_COND,
                        INTERCEPT_XSETBV,
                        INTERCEPT_RDPRU,
                        TRAP_EFER_WRITE,
                        TRAP_CR0_WRITE,
                        TRAP_CR1_WRITE,
                        TRAP_CR2_WRITE,
                        TRAP_CR3_WRITE,
                        TRAP_CR4_WRITE,
                        TRAP_CR5_WRITE,
                        TRAP_CR6_WRITE,
                        TRAP_CR7_WRITE,
                        TRAP_CR8_WRITE,
                        /* Byte offset 014h (word 5) */
                        INTERCEPT_INVLPGB = 160,
                        INTERCEPT_INVLPGB_ILLEGAL,
                        INTERCEPT_INVPCID,
                        INTERCEPT_MCOMMIT,
                        INTERCEPT_TLBSYNC,
                };

                struct __attribute__ ((__packed__)) vmcb_control_area {
                        uint32_t intercepts[MAX_INTERCEPT];
                        uint32_t reserved_1[15 - MAX_INTERCEPT];
                        uint16_t pause_filter_thresh;
                        uint16_t pause_filter_count;
                        uint64_t iopm_base_pa;
                        uint64_t msrpm_base_pa;
                        uint64_t tsc_offset;
                        uint32_t asid;
                        uint8_t tlb_ctl;
                        uint8_t reserved_2[3];
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
                        uint64_t nested_ctl;
                        uint64_t avic_vapic_bar;
                        uint64_t ghcb_gpa;
                        uint32_t event_inj;
                        uint32_t event_inj_err;
                        uint64_t nested_cr3;
                        uint64_t virt_ext;
                        uint32_t clean;
                        uint32_t reserved_5;
                        uint64_t next_rip;
                        uint8_t insn_len;
                        uint8_t insn_bytes[15];
                        uint64_t avic_backing_page;	/* Offset 0xe0 */
                        uint8_t reserved_6[8];	/* Offset 0xe8 */
                        uint64_t avic_logical_id;	/* Offset 0xf0 */
                        uint64_t avic_physical_id;	/* Offset 0xf8 */
                        uint8_t reserved_7[8];
                        uint64_t vmsa_pa;		/* Used for an SEV-ES guest */
                        uint8_t reserved_8[720];
                        /*
                         * Offset 0x3e0, 32 bytes reserved
                         * for use by hypervisor/software.
                         */
                        uint8_t reserved_sw[32];
                };

                /*
                 * The state-save area of the VMCB containing saved guest state
                 * accroding to "Appendix B" on page 736 at
                 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
                 */
                struct __attribute__ ((__packed__)) vmcb_seg {
                        uint16_t selector;
                        uint16_t attrib;
                        uint32_t limit;
                        uint64_t base;
                };

                /* Save area definition for legacy and SEV-MEM guests */
                struct __attribute__ ((__packed__)) vmcb_save_area {
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
                        uint8_t reserved_0xa0[42];
                        uint8_t vmpl;
                        uint8_t cpl;
                        uint8_t reserved_0xcc[4];
                        uint64_t efer;
                        uint8_t reserved_0xd8[112];
                        uint64_t cr4;
                        uint64_t cr3;
                        uint64_t cr0;
                        uint64_t dr7;
                        uint64_t dr6;
                        uint64_t rflags;
                        uint64_t rip;
                        uint8_t reserved_0x180[88];
                        uint64_t rsp;
                        uint64_t s_cet;
                        uint64_t ssp;
                        uint64_t isst_addr;
                        uint64_t rax;
                        uint64_t star;
                        uint64_t lstar;
                        uint64_t cstar;
                        uint64_t sfmask;
                        uint64_t kernel_gs_base;
                        uint64_t sysenter_cs;
                        uint64_t sysenter_esp;
                        uint64_t sysenter_eip;
                        uint64_t cr2;
                        uint8_t reserved_0x248[32];
                        uint64_t g_pat;
                        uint64_t dbgctl;
                        uint64_t br_from;
                        uint64_t br_to;
                        uint64_t last_excp_from;
                        uint64_t last_excp_to;
                        uint8_t reserved_0x298[72];
                        uint64_t spec_ctrl;		/* Guest version of SPEC_CTRL at 0x2E0 */
                };
                static_assert(sizeof(struct vmcb_control_area) == 1024);

                struct __attribute__ ((__packed__)) vmcb {
                        struct vmcb_control_area control;
                        struct vmcb_save_area save;
                };
                static_assert(sizeof(struct vmcb) <= 4096);

                #include <linux/mutex.h>
                #include "vm.h"
                /* virtual cpu data structure */
                struct vcpu {
                        struct mutex lock;
                        struct vmcb *vmcb;
                        void *hsave;
                        struct vm *vm;
                };

                /* create the vcpu */
                extern struct vcpu* yakvm_create_vcpu(struct vm *vm);

                /* destory the vcpu */
                extern void yakvm_destroy_vcpu(struct vcpu *vcpu);

                #include <linux/fs.h>
                extern const struct file_operations yakvm_vcpu_fops;

        #endif // __KERNEL__

        #include "../include/yakvm.h"
        /* ioctls for vcpu fds */
        #define YAKVM_RUN               _IO(YAKVMIO,   0x20)

        /* *vmexit* exit code */
        #define SVM_EXIT_READ_CR0                       0x000
        #define SVM_EXIT_READ_CR2                       0x002
        #define SVM_EXIT_READ_CR3                       0x003
        #define SVM_EXIT_READ_CR4                       0x004
        #define SVM_EXIT_READ_CR8                       0x008
        #define SVM_EXIT_WRITE_CR0                      0x010
        #define SVM_EXIT_WRITE_CR2                      0x012
        #define SVM_EXIT_WRITE_CR3                      0x013
        #define SVM_EXIT_WRITE_CR4                      0x014
        #define SVM_EXIT_WRITE_CR8                      0x018
        #define SVM_EXIT_READ_DR0                       0x020
        #define SVM_EXIT_READ_DR1                       0x021
        #define SVM_EXIT_READ_DR2                       0x022
        #define SVM_EXIT_READ_DR3                       0x023
        #define SVM_EXIT_READ_DR4                       0x024
        #define SVM_EXIT_READ_DR5                       0x025
        #define SVM_EXIT_READ_DR6                       0x026
        #define SVM_EXIT_READ_DR7                       0x027
        #define SVM_EXIT_WRITE_DR0                      0x030
        #define SVM_EXIT_WRITE_DR1                      0x031
        #define SVM_EXIT_WRITE_DR2                      0x032
        #define SVM_EXIT_WRITE_DR3                      0x033
        #define SVM_EXIT_WRITE_DR4                      0x034
        #define SVM_EXIT_WRITE_DR5                      0x035
        #define SVM_EXIT_WRITE_DR6                      0x036
        #define SVM_EXIT_WRITE_DR7                      0x037
        #define SVM_EXIT_EXCP_BASE                      0x040
        #define SVM_EXIT_LAST_EXCP                      0x05f
        #define SVM_EXIT_INTR                           0x060
        #define SVM_EXIT_NMI                            0x061
        #define SVM_EXIT_SMI                            0x062
        #define SVM_EXIT_INIT                           0x063
        #define SVM_EXIT_VINTR                          0x064
        #define SVM_EXIT_CR0_SEL_WRITE                  0x065
        #define SVM_EXIT_IDTR_READ                      0x066
        #define SVM_EXIT_GDTR_READ                      0x067
        #define SVM_EXIT_LDTR_READ                      0x068
        #define SVM_EXIT_TR_READ                        0x069
        #define SVM_EXIT_IDTR_WRITE                     0x06a
        #define SVM_EXIT_GDTR_WRITE                     0x06b
        #define SVM_EXIT_LDTR_WRITE                     0x06c
        #define SVM_EXIT_TR_WRITE                       0x06d
        #define SVM_EXIT_RDTSC                          0x06e
        #define SVM_EXIT_RDPMC                          0x06f
        #define SVM_EXIT_PUSHF                          0x070
        #define SVM_EXIT_POPF                           0x071
        #define SVM_EXIT_CPUID                          0x072
        #define SVM_EXIT_RSM                            0x073
        #define SVM_EXIT_IRET                           0x074
        #define SVM_EXIT_SWINT                          0x075
        #define SVM_EXIT_INVD                           0x076
        #define SVM_EXIT_PAUSE                          0x077
        #define SVM_EXIT_HLT                            0x078
        #define SVM_EXIT_INVLPG                         0x079
        #define SVM_EXIT_INVLPGA                        0x07a
        #define SVM_EXIT_IOIO                           0x07b
        #define SVM_EXIT_MSR                            0x07c
        #define SVM_EXIT_TASK_SWITCH                    0x07d
        #define SVM_EXIT_FERR_FREEZE                    0x07e
        #define SVM_EXIT_SHUTDOWN                       0x07f
        #define SVM_EXIT_VMRUN                          0x080
        #define SVM_EXIT_VMMCALL                        0x081
        #define SVM_EXIT_VMLOAD                         0x082
        #define SVM_EXIT_VMSAVE                         0x083
        #define SVM_EXIT_STGI                           0x084
        #define SVM_EXIT_CLGI                           0x085
        #define SVM_EXIT_SKINIT                         0x086
        #define SVM_EXIT_RDTSCP                         0x087
        #define SVM_EXIT_ICEBP                          0x088
        #define SVM_EXIT_WBINVD                         0x089
        #define SVM_EXIT_MONITOR                        0x08a
        #define SVM_EXIT_MWAIT                          0x08b
        #define SVM_EXIT_MWAIT_COND                     0x08c
        #define SVM_EXIT_XSETBV                         0x08d
        #define SVM_EXIT_RDPRU                          0x08e
        #define SVM_EXIT_EFER_WRITE_TRAP                0x08f
        #define SVM_EXIT_CR0_WRITE_TRAP                 0x090
        #define SVM_EXIT_CR1_WRITE_TRAP                 0x091
        #define SVM_EXIT_CR2_WRITE_TRAP                 0x092
        #define SVM_EXIT_CR3_WRITE_TRAP                 0x093
        #define SVM_EXIT_CR4_WRITE_TRAP                 0x094
        #define SVM_EXIT_CR5_WRITE_TRAP                 0x095
        #define SVM_EXIT_CR6_WRITE_TRAP                 0x096
        #define SVM_EXIT_CR7_WRITE_TRAP                 0x097
        #define SVM_EXIT_CR8_WRITE_TRAP                 0x098
        #define SVM_EXIT_CR9_WRITE_TRAP                 0x099
        #define SVM_EXIT_CR10_WRITE_TRAP                0x09a
        #define SVM_EXIT_CR11_WRITE_TRAP                0x09b
        #define SVM_EXIT_CR12_WRITE_TRAP                0x09c
        #define SVM_EXIT_CR13_WRITE_TRAP                0x09d
        #define SVM_EXIT_CR14_WRITE_TRAP                0x09e
        #define SVM_EXIT_CR15_WRITE_TRAP                0x09f
        #define SVM_EXIT_INVPCID                        0x0a2
        #define SVM_EXIT_NPF                            0x400
        #define SVM_EXIT_AVIC_INCOMPLETE_IPI            0x401
        #define SVM_EXIT_AVIC_UNACCELERATED_ACCESS      0x402
        #define SVM_EXIT_VMGEXIT                        0x403
        #define SVM_EXIT_ERR                            -1

#endif // __YAKVM_CPU_H_
