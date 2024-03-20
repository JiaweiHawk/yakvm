#ifndef __YAKVM_VM_H_

        #define __YAKVM_VM_H_

        #ifdef __KERNEL__
                #include <linux/mutex.h>
                #include <linux/types.h>
                #include <linux/xarray.h>
                #define YAKVM_VM_MAX_ID         32
                /* virtual machine data structure */
                struct vm {
                        struct mutex lock;
                        atomic_t refcount;
                        struct vcpu *vcpu;
                        char id[YAKVM_VM_MAX_ID];
                };

                /* create the vm */
                extern struct vm* yakvm_create_vm(void);

                /* destory the vm */
                extern void yakvm_destroy_vm(struct vm *vm);

                /* put the vm */
                extern void yakvm_put_vm(struct vm *vm);

                #include <linux/fs.h>
                /* interface for userspace-kvm interaction */
                extern const struct file_operations yakvm_vm_fops;
        #endif //__KERNEL__

        #include "../include/yakvm.h"
        /* ioctls for vm fds */
        #define YAKVM_CREATE_VCPU       _IO(YAKVMIO,   0x10) /* returns a vcpu fd */

#endif // __YAKVM_VM_H_
