#ifndef __YAKVM_VM_H_

    #define __YAKVM_VM_H_

    #ifdef __KERNEL__
        /* virtual machine data structure */
        #define YAKVM_VM_MAX_ID         32
        struct vm {
            char id[YAKVM_VM_MAX_ID];
        };

        /* create the vm */
        extern struct vm* yakvm_create_vm(void);

        /* destory the vm */
        extern void yakvm_destroy_vm(struct vm *vm);

        #include <linux/fs.h>
        /* interface for userspace-kvm interaction */
        extern const struct file_operations yakvm_vm_fops;
    #endif //__KERNEL__

#endif // __YAKVM_VM_H_
