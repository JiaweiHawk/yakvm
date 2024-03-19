#include <linux/err.h>
#include "../include/vm.h"

/*
 * interface for userspace-kvm interaction, describe how the
 * userspace emulator can manipulate the virtual machine
 */
const struct file_operations yakvm_vm_fops = {

};

/* create the vm */
struct vm *yakvm_create_vm(const char *fdname)
{
    return ERR_PTR(-ENOSYS);
}


/* put the vm */
void yakvm_put_vm(struct vm *vm)
{
}
