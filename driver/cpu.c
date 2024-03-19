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
struct vcpu *yakvm_create_vcpu(struct vm *vm)
{
    struct vcpu *vcpu;

    vcpu = kmalloc(sizeof(struct vcpu), GFP_KERNEL_ACCOUNT | __GFP_ZERO);
    if (!vcpu) {
        log(LOG_ERR, "kmalloc() failed");
        return ERR_PTR(-ENOMEM);
    }

    /* initialize the vcpu */
    vcpu->vm = vm;

    return vcpu;
}

/* destroy the vcpu */
void yakvm_destroy_vcpu(struct vcpu *vcpu)
{
    kfree(vcpu);
}
