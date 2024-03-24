#include <asm/io.h>
#include <asm/msr.h>
#include <asm/page_types.h>
#include <asm/processor-flags.h>
#include <asm-generic/bitops/instrumented-non-atomic.h>
#include <linux/gfp.h>
#include <linux/gfp_types.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/preempt.h>
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
 * When the *vmrun* instruction exits(back to the host), an
 * exit/reason code is stored in the *EXITCODE* field in the
 * *vmcb* according to *Appendix C* on page 745 at
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */
static int yakvm_vcpu_handle_exit(struct vcpu *vcpu)
{
        uint32_t exit_code = vcpu->gvmcb->control.exit_code;
        assert(exit_code == SVM_EXIT_ERR);
        return exit_code;
}

/*
 * run virtual machine with *vmrun* according to
 * "15.5" on page 81 at
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */
static int yakvm_vcpu_run(struct vcpu *vcpu)
{

        /*
         * This ensures that the vcpu is binded on the physical cpu
         * instead of being scheduled to other physical cpus.
         */
        preempt_disable();

        /*
         * physical cpu *VM_HSAVE_PA* MSR holds the physical address
         * of a block of memory where *vmrun* save host state and from
         * which *vmexit* reloads host state according to "15.30.4" on
         * page 585 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
         *
         * Note that this setting is pcpu specific, so is should be
         * setted after the vcpu is binded to the pcpu.
         */
        wrmsrl(MSR_VM_HSAVE_PA, virt_to_phys(vcpu->hsave));

        /*
         * Considering that *vmrun* and *vmexit* only saves part or none
         * of host state, kernel should also use *vmsave* and *vmload*
         * to restore the totaly host state according to "15.5.1"
         * on page 501 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        asm volatile (
                "vmsave %0\n\t"
                :
                :"a"(virt_to_phys(vcpu->hvmcb))
                :"memory"
        );

        asm volatile (
                /* enter guest mode */
                "vmrun %0\n\t"
                :
                :"a"(virt_to_phys(vcpu->gvmcb))
                :"cc", "memory"
        );

        asm volatile (
                "vmload %0\n\t"
                :
                :"a"(virt_to_phys(vcpu->hvmcb))
                :"cc"
        );

        preempt_enable();

        /* handle the *vmexit* */
        return yakvm_vcpu_handle_exit(vcpu);
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

static inline void yakvm_vmcb_init_segment_register(
        struct vmcb_seg *seg, uint16_t selector, uint16_t attrib,
        uint32_t limit, uint64_t base)
{
	seg->selector = selector;
	seg->attrib = attrib;
	seg->limit = limit;
	seg->base = base;
}

static inline void yakvm_vmcb_init_dt_register(struct vmcb_seg *seg,
                                        uint32_t limit, uint64_t base)
{
        seg->limit = limit;
        seg->base = base;
}

/*
 * initialize the *vmcb* for guest state according to "15.5" on page 501 at
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */
static void yakvm_vcpu_init_vmcb(struct vmcb *vmcb)
{
        /*
         * initialize the guest to the initial processor state
         * according to "14.1.3" on page 480 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        vmcb->save.cr0 = X86_CR0_NW | X86_CR0_CD | X86_CR0_ET;
        vmcb->save.rflags = X86_EFLAGS_FIXED;
        vmcb->save.rip = 0xfff0;
        yakvm_vmcb_init_segment_register(&vmcb->save.cs, 0xf000,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_CODE_MASK,
                0xffff, 0xffff0000);
        yakvm_vmcb_init_segment_register(&vmcb->save.ds, 0,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_WRITE_MASK,
                0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.es, 0,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_WRITE_MASK,
                0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.fs, 0,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_WRITE_MASK,
                0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.gs, 0,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_WRITE_MASK,
                0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.ss, 0,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_WRITE_MASK,
                0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.ss, 0,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_WRITE_MASK,
                0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.ss, 0,
                SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
                SVM_SELECTOR_READ_MASK | SVM_SELECTOR_WRITE_MASK,
                0xffff, 0x0);
        yakvm_vmcb_init_dt_register(&vmcb->save.gdtr, 0xffff, 0);
        yakvm_vmcb_init_dt_register(&vmcb->save.idtr, 0xffff, 0);
        yakvm_vmcb_init_segment_register(&vmcb->save.ldtr, 0,
                                        SEG_TYPE_LDT, 0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.ldtr, 0,
                                        SEG_TYPE_BUSY_TSS16, 0xffff, 0x0);
        vmcb->save.dr6 = DR6_ACTIVE_LOW;
        vmcb->save.dr7 = DR7_FIXED_1;
}

/* create the vcpu */
struct vcpu* yakvm_create_vcpu(struct vm *vm)
{
        struct vcpu *vcpu;
        struct vmcb *gvmcb, *hvmcb;
        void *hsave;
        void *ret;

        vcpu = kmalloc(sizeof(struct vcpu), GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!vcpu) {
                log(LOG_ERR, "kmalloc() failed");
                ret = ERR_PTR(-ENOMEM);
                goto out;
        }

        /*
         * Virtual Machine Control Block(vmcb) should be a
         * 4KB-aligned page which describes a guest to be executed
         * accroding to "15.5" on page 500 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        gvmcb = kmalloc(4096, GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!gvmcb) {
                log(LOG_ERR, "kmalloc() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_vcpu;
        }

        /*
         * Considering that *vmrun* and *vmexit* only saves part or none
         * of host state, kernel should also use *vmcb* with
         * *vmsave* and *vmload* to restore the totaly host state
         * according to "15.5.1" on page 501 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        hvmcb = kmalloc(4096, GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!hvmcb) {
                log(LOG_ERR, "kmalloc() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_gvmcb;
        }

        /*
         * *hsave* is a 4KB block of memory where *vmrun* saves part or
         * none of host state, and from which *vmexit* reloads saving
         * host state according to "15.30.4" on page 585 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        hsave = kmalloc(4096, GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!hsave) {
                log(LOG_ERR, "kmalloc() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_hvmcb;
        }

        /* initialize the vcpu */
        mutex_init(&vcpu->lock);
        yakvm_vcpu_init_vmcb(gvmcb);
        vcpu->gvmcb = gvmcb;
        /*
         * kernel needs to initialize the *gvmcb* to setup the
         * guest. However, it does not need to initialize the
         * hvmcb* as it is only used to temporarily store the
         * host state during guest execution.
         */
        vcpu->hvmcb = hvmcb;
        vcpu->hsave = hsave;
        vcpu->vm = vm;

        return vcpu;

free_hvmcb:
        kfree(hvmcb);
free_gvmcb:
        kfree(gvmcb);
free_vcpu:
        kfree(vcpu);
out:
        return ret;
}

/* destroy the vcpu */
void yakvm_destroy_vcpu(struct vcpu *vcpu)
{
        kfree(vcpu->hsave);
        kfree(vcpu->hvmcb);
        kfree(vcpu->gvmcb);
        kfree(vcpu);
}
