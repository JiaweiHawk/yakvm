#include <linux/gfp_types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "../include/cpu.h"
#include "../include/vm.h"
#include "../include/yakvm.h"

static int yakvm_vcpu_release(struct inode *inode, struct file *filp)
{
        struct vcpu *vcpu = filp->private_data;

        yakvm_put_vm(vcpu->vm);
        return 0;
}

/*
 * interface for userspace-vcpu interaction, describe how the
 * userspace emulator can manipulate the virtual cpu
 */
const struct file_operations yakvm_vcpu_fops = {
        .release = yakvm_vcpu_release,
};

/* create the vcpu */
struct vcpu* yakvm_create_vcpu(struct vm *vm)
{
        struct vcpu *vcpu;
        struct vmcb *vmcb;

        vcpu = kmalloc(sizeof (struct vcpu), GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!vcpu) {
                log(LOG_ERR, "kmalloc() failed");
                return ERR_PTR(-ENOMEM);
        }

        /*
         * Virtual Machine Control Block(vmcb) should be a
         * 4KB-aligned page accroding to "2.2" on page 5 at
         * https://www.0x04.net/doc/amd/33047.pdf
         */
        vmcb = kmalloc(4096, GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!vmcb) {
                log(LOG_ERR, "kmalloc() failed");
                kfree(vcpu);
                return ERR_PTR(-ENOMEM);
        }

        /* initialize the vcpu */
        vcpu->vmcb = vmcb;
        vcpu->vm = vm;

        return vcpu;
}

/* destroy the vcpu */
void yakvm_destroy_vcpu(struct vcpu *vcpu)
{
        kfree(vcpu->vmcb);
        kfree(vcpu);
}
