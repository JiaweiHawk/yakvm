#include <linux/err.h>
#include <linux/gfp_types.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include "../include/vm.h"
#include "../include/yakvm.h"

static int yakvm_vm_release(struct inode *inode, struct file *filp)
{
	struct vm *vm = filp->private_data;
	yakvm_destroy_vm(vm);
	return 0;
}

/*
 * interface for userspace-kvm interaction, describe how the
 * userspace emulator can manipulate the virtual machine
 */
const struct file_operations yakvm_vm_fops = {
	.release = yakvm_vm_release,
};

/* create the vm */
struct vm *yakvm_create_vm()
{
	struct vm *vm = kmalloc(sizeof(struct vm), GFP_KERNEL_ACCOUNT | __GFP_ZERO);
	if (!vm) {
		log(LOG_ERR, "kmalloc() failed");
		return ERR_PTR(-ENOMEM);
	}

	snprintf(vm->id, sizeof(vm->id), "kvm-%d", task_pid_nr(current));
	log(LOG_INFO, "create the kvm %s", vm->id);
	return vm;
}

/* destroy the vm */
void yakvm_destroy_vm(struct vm *vm)
{
	log(LOG_INFO, "destroy the kvm %s", vm->id);
    kfree(vm);
}
