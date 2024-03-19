#ifndef __YAKVM_CPU_H_

    #define __YAKVM_CPU_H_

    #ifdef __KERNEL__
        #include "vm.h"
        /* virtual cpu data structure */
        struct vcpu {
            struct vm *vm;
        };

        /* create the vcpu */
        extern struct vcpu* yakvm_create_vcpu(struct vm *vm);

        /* destory the vcpu */
        extern void yakvm_destroy_vcpu(struct vcpu *vcpu);

        #include <linux/fs.h>
        extern const struct file_operations yakvm_vcpu_fops;

    #endif // __KERNEL__

    #define KVM_CREATE_VCPU           _IO(KVMIO,   0x41)

#endif // __YAKVM_CPU_H_
