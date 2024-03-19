#ifndef __YAKVM_VM_H_

    #define __YAKVM_VM_H_

    #ifdef __KERNEL__
        /* virtual machine data structure */
        struct vm {
        };

        /* create the vm */
        extern struct vm* yakvm_create_vm(const char *fdname);

        /* put the vm */
        extern void yakvm_put_vm(struct vm *vm);

        #include <linux/fs.h>
        /* interface for userspace-kvm interaction */
        extern const struct file_operations yakvm_vm_fops;
    #endif //__KERNEL__

#endif // __YAKVM_VM_H_