#include <asm/io.h>
#include <asm/msr.h>
#include <asm/page_types.h>
#include <asm/processor-flags.h>
#include <linux/bitops.h>
#include <linux/gfp.h>
#include <linux/gfp_types.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/preempt.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/vm.h"
#include "../include/yakvm.h"

static atomic_t asid = ATOMIC_INIT(1);

static int yakvm_vcpu_release(struct inode *inode, struct file *filp)
{
        struct vcpu *vcpu = filp->private_data;

        yakvm_put_vm(vcpu->vm);
        return 0;
}

/*
 * share error information from vcpu to userspace, so that
 * virtual machine error can be handled in userspace.
 */
static void yakvm_vcpu_share_err_to_user(struct vcpu *vcpu)
{
        struct state *state = vcpu->state;
        struct vmcb_control_area *control = &vcpu->gctx.vmcb->control;
        struct vmcb_save_area *save = &vcpu->gctx.vmcb->save;

        state->exit_code = control->exit_code;
        state->exit_info_1 = control->exit_info_1;
        state->exit_info_2 = control->exit_info_2;
        state->cs = save->cs.base;
        state->rip = save->rip;
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
         * It is assumed that kernel cleared *gif* some time before
         * executing the *vmrun* instruction to ensure an atomic
         * state switch according to "15.5.1" on page 502 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
         */
        asm volatile (
                "clgi\n\t"
        );

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
                :"a"(virt_to_phys(vcpu->hctx.vmcb))
                :"memory"
        );

        /* enter guest */
        _yakvm_vcpu_run(&vcpu->gctx, &vcpu->hctx, virt_to_phys(vcpu->gctx.vmcb));

        asm volatile (
                "vmload %0\n\t"
                :
                :"a"(virt_to_phys(vcpu->hctx.vmcb))
                :"cc"
        );

        asm volatile (
                "stgi\n\t"
        );

        preempt_enable();

        yakvm_vcpu_share_err_to_user(vcpu);
        return 0;
}

/* copy vcpu registers state to userspace */
static int yakvm_vcpu_get_regs(struct vcpu *vcpu,
                               void * __user dest)
{
        struct registers regs;
        int r;

        regs.rax = vcpu->gctx.vmcb->save.rax;
        regs.rbx = vcpu->gctx.rbx;
        regs.rcx = vcpu->gctx.rcx;
        regs.rdx = vcpu->gctx.rdx;
        regs.rdi = vcpu->gctx.rdi;
        regs.rsi = vcpu->gctx.rsi;
        regs.rbp = vcpu->gctx.rbp;
        regs.rsp = vcpu->gctx.vmcb->save.rsp;
        regs.r8 = vcpu->gctx.r8;
        regs.r9 = vcpu->gctx.r9;
        regs.r10 = vcpu->gctx.r10;
        regs.r11 = vcpu->gctx.r11;
        regs.r12 = vcpu->gctx.r12;
        regs.r13 = vcpu->gctx.r13;
        regs.r14 = vcpu->gctx.r14;
        regs.r15 = vcpu->gctx.r15;
        regs.rip = vcpu->gctx.vmcb->save.rip;
        regs.cs = vcpu->gctx.vmcb->save.cs.base;

        r = copy_to_user(dest, &regs, sizeof(regs));
        if (r) {
                log(LOG_ERR, "copy_to_user() failed with %d bytes", r);
                return -EFAULT;
        }

        return 0;
}

/* copy vcpu registers state from userspace */
static int yakvm_vcpu_set_regs(struct vcpu *vcpu,
                               void * __user src)
{
        struct registers regs;
        int r;

        r = copy_from_user(&regs, src, sizeof(regs));
        if (r) {
                log(LOG_ERR, "copy_from_user() failed with %d bytes", r);
                return -EFAULT;
        }

        vcpu->gctx.vmcb->save.rax = regs.rax;
        vcpu->gctx.rbx = regs.rbx;
        vcpu->gctx.rcx = regs.rcx;
        vcpu->gctx.rdx = regs.rdx;
        vcpu->gctx.rdi = regs.rdi;
        vcpu->gctx.rsi = regs.rsi;
        vcpu->gctx.rbp = regs.rbp;
        vcpu->gctx.vmcb->save.rsp = regs.rsp;
        vcpu->gctx.r8 = regs.r8;
        vcpu->gctx.r9 = regs.r9;
        vcpu->gctx.r10 = regs.r10;
        vcpu->gctx.r11 = regs.r11;
        vcpu->gctx.r12 = regs.r12;
        vcpu->gctx.r13 = regs.r13;
        vcpu->gctx.r14 = regs.r14;
        vcpu->gctx.r15 = regs.r15;
        vcpu->gctx.vmcb->save.rip = regs.rip;
        vcpu->gctx.vmcb->save.cs.base = regs.cs;

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

                case YAKVM_GET_REGS:
                        r = yakvm_vcpu_get_regs(vcpu, (void *)arg);
                        break;

                case YAKVM_SET_REGS:
                        r = yakvm_vcpu_set_regs(vcpu, (void *)arg);
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

/* share the vcpu state with kernel and userspace */
static int yakvm_vcpu_state_mmap(struct file *filp,
                                 struct vm_area_struct *vma)
{
        int r;
        struct vcpu *vcpu = filp->private_data;

        if (vma->vm_pgoff) {
                log(LOG_ERR, "yakvm_vcpu_state_mmap() map at %ld "
                    "instead of 0", vma->vm_pgoff);
                return -EINVAL;
        }

	if (vma->vm_end - vma->vm_start != PAGE_SIZE) {
                log(LOG_ERR, "yakvm_vcpu_state_mmap() map %ld bytes "
                    "instead of page size", vma->vm_end - vma->vm_start);
                return -EINVAL;
        }

        r = remap_pfn_range(vma, vma->vm_start,
                            virt_to_phys(vcpu->state) >> PAGE_SHIFT,
                            PAGE_SIZE, vma->vm_page_prot);
        if (r < 0) {
                log(LOG_ERR, "remap_pfn_range() failed with error code %d",
                    r);
                return r;
        }

        return 0;
}

/*
 * interface for userspace-vcpu interaction, describe how the
 * userspace emulator can manipulate the virtual cpu
 */
const struct file_operations yakvm_vcpu_fops = {
        .release = yakvm_vcpu_release,
        .unlocked_ioctl = yakvm_vcpu_ioctl,
        .mmap = yakvm_vcpu_state_mmap,
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

static inline void yakvm_vmcb_init_descriptor_table_register(
                                        struct vmcb_seg *seg, uint32_t limit,
                                        uint64_t base)
{
        seg->limit = limit;
        seg->base = base;
}

static inline void yakvm_vmcb_set_intercept(struct vmcb *vmcb,
                                            uint32_t bit)
{
        assert(bit < 32 * MAX_INTERCEPT);
        __set_bit(bit, (unsigned long *)&vmcb->control.intercepts);
}

static inline void yakvm_vmcb_set_exception_intercept(struct vmcb *vmcb,
                                            uint32_t bit)
{
        yakvm_vmcb_set_intercept(vmcb, INTERCEPT_EXCEPTION_OFFSET + bit);
}

/*
 * initialize the *vmcb* for guest state according to "15.5" on page 501 at
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */
static void yakvm_vcpu_init_vmcb(struct vcpu *vcpu)
{
        /*
         * kernel needs to initialize the *gvmcb* to setup the
         * guest. However, it does not need to initialize the
         * hvmcb* as it is only used to temporarily store the
         * host state during guest execution.
         */
        struct vmcb *vmcb = vcpu->gctx.vmcb;

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
        yakvm_vmcb_init_descriptor_table_register(&vmcb->save.gdtr,
                                                  0xffff, 0);
        yakvm_vmcb_init_descriptor_table_register(&vmcb->save.idtr,
                                                  0xffff, 0);
        yakvm_vmcb_init_segment_register(&vmcb->save.ldtr, 0,
                                        SEG_TYPE_LDT, 0xffff, 0x0);
        yakvm_vmcb_init_segment_register(&vmcb->save.tr, 0,
                                        SEG_TYPE_BUSY_TSS16, 0xffff, 0x0);
        vmcb->save.dr6 = DR6_ACTIVE_LOW;
        vmcb->save.dr7 = DR7_FIXED_1;

        /*
         * Enable the *X86_EFLAGS_TF* to enable single-step mode for
         * debugging the guest code according to "13.1.4" on page 407 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        vmcb->save.rflags |= X86_EFLAGS_TF;
        yakvm_vmcb_set_exception_intercept(vmcb, DB_VECTOR);

        /*
         * with Nested Paging Table(NPT) enabled, the nested page table,
         * residing in system physical memory and pointed to by nCR3,
         * mapping guest physical addresses to system physical addresses
         * according to "15.25.1" on page 547 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         *
         * Enable npt according to "15.25.3" on page 549 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        vmcb->control.nested_ctl |= SVM_NESTED_CTL_NP_ENABLE;
        vmcb->control.nested_cr3 = vcpu->vm->vmm->ncr3;

        /*
         * *vmrun* instruction performs consistency checks on guest state,
         * illegal guest state cause a *vmexit* with error code
         * *VMEXIT_INVALID* according to "15.5.1" on page 503
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        vmcb->save.efer |= EFER_SVME;
        yakvm_vmcb_set_intercept(vmcb, INTERCEPT_VMRUN);      /* current
        implementation requires that the *vmrun* intercept always be set
        in the *vmcb* according to "15.9" on page 514 at
        https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf */
        vmcb->control.asid = atomic_inc_return(&asid);          /* ensure
        different guests can coexist in the TLB according to
        "15.25.1" on page 548 at
        https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf */
        assert(vmcb->control.asid < cpuid_ebx(0x8000000a));    /* according
        to "15.5.1" on page 503 at
        https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf */
}

/* create the vcpu */
struct vcpu* yakvm_create_vcpu(struct vm *vm)
{
        struct page *state, *gvmcb, *hvmcb, *hsave;
        struct vcpu *vcpu;
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
        gvmcb = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!gvmcb) {
                log(LOG_ERR, "alloc_page() failed");
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
        hvmcb = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!hvmcb) {
                log(LOG_ERR, "alloc_page() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_gvmcb;
        }

        /*
         * *hsave* is a 4KB block of memory where *vmrun* saves part or
         * none of host state, and from which *vmexit* reloads saving
         * host state according to "15.30.4" on page 585 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
         */
        hsave = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!hsave) {
                log(LOG_ERR, "alloc_page() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_hvmcb;
        }

	state = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
        if (!state) {
                log(LOG_ERR, "alloc_page() failed");
                ret = ERR_PTR(-ENOMEM);
                goto free_hsave;
        }

        /* initialize the vcpu */
        mutex_init(&vcpu->lock);
        vcpu->gctx.vmcb = page_address(gvmcb);
        vcpu->hctx.vmcb = page_address(hvmcb);
        vcpu->hsave = page_address(hsave);
        vcpu->state = page_address(state);
        vcpu->vm = vm;
        yakvm_vcpu_init_vmcb(vcpu);

        return vcpu;

free_hsave:
        __free_page(hsave);
free_hvmcb:
        __free_page(hvmcb);
free_gvmcb:
        __free_page(gvmcb);
free_vcpu:
        kfree(vcpu);
out:
        return ret;
}

/* destroy the vcpu */
void yakvm_destroy_vcpu(struct vcpu *vcpu)
{
        free_page((unsigned long)vcpu->state);
        free_page((unsigned long)vcpu->hsave);
        free_page((unsigned long)vcpu->hctx.vmcb);
        free_page((unsigned long)vcpu->gctx.vmcb);
        kfree(vcpu);
}
