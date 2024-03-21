#include <asm/io.h>
#include <asm/msr.h>
#include <asm/page_types.h>
#include <linux/gfp.h>
#include <linux/gfp_types.h>
#include <linux/mm.h>
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
 * run virtual machine with *vmrun* according to
 * "4.2" on page 81 at
 * https://www.0x04.net/doc/amd/33047.pdf
 */
static int yakvm_vcpu_run(struct vcpu *vcpu) {

        /*
         * clears the global interrupt flag, so all external interrupts
         * are disabled according to "4.2" on page 72 at
         * https://www.0x04.net/doc/amd/33047.pdf.
         *
         * This ensures that the vcpu is binded on the physical cpu
         * instead of being scheduled to other physical cpus during
         * vmrun due to interrupts.
         */
        asm volatile (
                "clgi\n\t"
        );

        /*
         * physical cpu *VM_HSAVE_PA* MSR holds the physical address
         * of a block of memory where *vmrun* save host state and from
         * which *vmexit* reloads host state according to "E.4" on
         * page 109 at https://www.0x04.net/doc/amd/33047.pdf.
         *
         * Note that this setting is pcpu specific, so is should be
         * setted after the vcpu is binded to the pcpu.
         */
        wrmsrl(MSR_VM_HSAVE_PA, page_to_pfn(vcpu->hsave) << PAGE_SHIFT);

        asm volatile (
                "movq %0, %%rax\n\t"
                "vmrun\n\t"
                :
                :"r"(virt_to_phys(vcpu->vmcb))
        );

        /* enable all external interrupts */
        asm volatile (
                "stgi\n\t"
        );

        return 0;
}

static long yakvm_vcpu_ioctl(struct file *filp, unsigned int ioctl,
                             unsigned long arg)
{
        struct vcpu *vcpu = filp->private_data;
        int r = 0;

        if (mutex_lock_killable(&vcpu->lock))
                return -EINTR;

        switch (ioctl) {
                case YAKVM_RUN:
                        r = yakvm_vcpu_run(vcpu);
                        break;
                default:
                        log(LOG_ERR, "yakvm_vm_ioctl() get unknown ioctl %d",
                            ioctl);
                        r = -EINVAL;
                        break;
        }

        mutex_unlock(&vcpu->lock);
        return r;
}

/*
 * interface for userspace-vcpu interaction, describe how the
 * userspace emulator can manipulate the virtual cpu
 */
const struct file_operations yakvm_vcpu_fops = {
        .release = yakvm_vcpu_release,
        .unlocked_ioctl = yakvm_vcpu_ioctl,
};

/* create the vcpu */
struct vcpu* yakvm_create_vcpu(struct vm *vm)
{
        struct vcpu *vcpu;
        struct vmcb *vmcb;
        struct page *hsave;
        void *ret;

        vcpu = kmalloc(sizeof(struct vcpu), GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!vcpu) {
                log(LOG_ERR, "kmalloc() failed");
                ret = ERR_PTR(-ENOMEM);
                goto out;
        }

        /*
         * Virtual Machine Control Block(vmcb) should be a
         * 4KB-aligned page accroding to "2.2" on page 5 at
         * https://www.0x04.net/doc/amd/33047.pdf
         */
        vmcb = kmalloc(4096, GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!vmcb) {
                log(LOG_ERR, "kmalloc() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_vcpu;
        }

        /*
         * *vmrun* saves host state to *hsave*, *vmexit* reloads host state
         * from *hsave* according to "E.4" on page 109 at
         * https://www.0x04.net/doc/amd/33047.pdf
         */
        hsave = alloc_page(GFP_KERNEL);
        if (!hsave) {
                log(LOG_ERR, "alloc_page() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_vmcb;
        }

        /* initialize the vcpu */
        mutex_init(&vcpu->lock);
        vcpu->vmcb = vmcb;
        vcpu->hsave = hsave;
        vcpu->vm = vm;

        return vcpu;

free_vmcb:
        kfree(vmcb);
free_vcpu:
        kfree(vcpu);
out:
        return ret;
}

/* destroy the vcpu */
void yakvm_destroy_vcpu(struct vcpu *vcpu)
{
        kfree(vcpu->vmcb);
        __free_page(vcpu->hsave);
        kfree(vcpu);
}
