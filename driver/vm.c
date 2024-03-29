#include <asm/atomic.h>
#include <linux/anon_inodes.h>
#include <linux/err.h>
#include <linux/fdtable.h>
#include <linux/gfp_types.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/vm.h"
#include "../include/yakvm.h"

/* get the vm */
static void yakvm_get_vm(struct vm *vm)
{
        atomic_inc(&vm->refcount);
}

/* destroy the vm and relative resources */
void yakvm_destroy_vm(struct vm *vm)
{
        log(LOG_INFO, "yakvm_destroy_vm() destroys the kvm %s", vm->id);
        yakvm_destroy_vmm(vm->vmm);
        if (vm->vcpu) {
                yakvm_destroy_vcpu(vm->vcpu);
        }
        kfree(vm);
}

/* put the vm */
void yakvm_put_vm(struct vm *vm)
{
        if (atomic_dec_and_test(&vm->refcount)) {
                yakvm_destroy_vm(vm);
        }
}

static int yakvm_vm_release(struct inode *inode, struct file *filp)
{
        struct vm *vm = filp->private_data;
        yakvm_put_vm(vm);
        return 0;
}

/* create the vcpu and return the vcpu fd */
static int yakvm_vm_ioctl_create_vcpu(struct vm *vm)
{
        char fdname[YAKVM_MAX_FDNAME];
        struct vcpu *vcpu;
        int fd, r;

        vcpu = yakvm_create_vcpu(vm);
        if (IS_ERR(vcpu)) {
                r = PTR_ERR(vm);
                log(LOG_ERR, "yakvm_create_vcpu() failed "
                    "with error code %d", r);
                goto out;
        }

        mutex_lock(&vm->lock);
        if (vm->vcpu) {
                mutex_unlock(&vm->lock);
                r = -EEXIST;
                log(LOG_ERR, "vcpu has been created for kvm %s",
                    vm->id);
                goto destroy_vcpu;
        }

        vm->vcpu = vcpu;
        mutex_unlock(&vm->lock);
        yakvm_get_vm(vm);

        snprintf(fdname, sizeof (fdname), "kvm-vcpu");
        fd = anon_inode_getfd(fdname, &yakvm_vcpu_fops, vcpu, O_RDWR);
        if (fd < 0) {
                r = fd;
                log(LOG_ERR, "anon_inode_getfd() failed "
                    "with error code %d", r);
                goto put_vm;
        }

        return fd;

put_vm:
        mutex_lock(&vm->lock);
        vm->vcpu = NULL;
        mutex_unlock(&vm->lock);
        yakvm_put_vm(vm);
destroy_vcpu:
        yakvm_destroy_vcpu(vcpu);
out:
        return r;
}

static long yakvm_vm_ioctl(struct file *filp, unsigned int ioctl, unsigned long arg)
{
        struct vm *vm = filp->private_data;
        int r;

        switch (ioctl) {
                case YAKVM_CREATE_VCPU:
                        r = yakvm_vm_ioctl_create_vcpu(vm);
                        if (r < 0) {
                                log(LOG_ERR,
                                     "yakvm_dev_ioctl_create_vm() "
                                     "failed with error code %d", r);
                        }
                        return r;

                default:
                        log(LOG_ERR, "yakvm_vm_ioctl() get unknown ioctl %d",
                            ioctl);
                        return -EINVAL;
        }

        return 0;
}

/* map the vm physical memory to host physical memory */
static vm_fault_t yakvm_vm_vmm_fault(struct vm_fault *vmf)
{
        return 0;
}

static const struct vm_operations_struct yakvm_vm_vmm_ops = {
        .fault = yakvm_vm_vmm_fault,
};

/* expose vm physical memory to userspace */
static int yakvm_vm_mmap(struct file *filp,
                         struct vm_area_struct *vma)
{
        vma->vm_ops = &yakvm_vm_vmm_ops;
        return 0;
}

/*
 * interface for userspace-kvm interaction, describe how the
 * userspace emulator can manipulate the virtual machine
 */
const struct file_operations yakvm_vm_fops = {
        .release = yakvm_vm_release,
        .unlocked_ioctl = yakvm_vm_ioctl,
        .mmap = yakvm_vm_mmap,
};

/* create the vm */
struct vm * yakvm_create_vm(void)
{
        struct vm *vm;
        struct vmm *vmm;
        int r;

        vm = kmalloc(sizeof(struct vm), GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!vm) {
                log(LOG_ERR, "kmalloc() failed");
                r = -ENOMEM;
                goto out;
        }

        vmm = yakvm_create_vmm(vm);
        if (IS_ERR(vmm)) {
                r = PTR_ERR(vmm);
                log(LOG_ERR, "yakvm_create_vmm() failed with error code "
                    "%d", r);
                goto free_vm;
        }

        /* initialize the *vm* */
        mutex_init(&vm->lock);
        atomic_set(&vm->refcount, 1);
        snprintf(vm->id, sizeof(vm->id), "kvm-%d", task_pid_nr(current));
        vm->vmm = vmm;

        log(LOG_INFO, "yakvm_create_vm() creates the kvm %s", vm->id);
        return vm;

free_vm:
        kfree(vm);
out:
        return ERR_PTR(r);
}
