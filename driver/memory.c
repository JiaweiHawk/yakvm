#include <asm/io.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/gfp_types.h>
#include <linux/slab.h>
#include "../include/memory.h"
#include "../include/vm.h"
#include "../include/yakvm.h"

struct vmm *yakvm_create_vmm(struct vm *vm)
{
    int r;
    struct vmm *vmm;
    struct page *pml4t;

    /*
     * nested paging uses the same paging mode as the host used when
     * it executed the *vmrun* according to "15.25.3" on page 549 at
     * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
     *
     * host typically uses 4-Kbyte level-4 long-mode page translation
     * to map 64-bit virtual address into 52-bit physical addresses
     * according to "5.3" on page 139 at
     * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
     */
    pml4t = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
    if (!pml4t) {
        log(LOG_ERR, "alloc_page() failed");
        r = -ENOMEM;
        goto out;
    }

    vmm = kmalloc(sizeof(*vmm), GFP_KERNEL_ACCOUNT | __GFP_ZERO);
    if (!vmm) {
        log(LOG_ERR, "kmalloc() failed");
        r = -ENOMEM;
        goto free_pml4;
    }

    /* initialize the vmm */
    vmm->ncr3 = page_to_phys(pml4t);
    vmm->vm = vm;

    return vmm;

free_pml4:
    __free_page(pml4t);
out:
    return ERR_PTR(r);
}

void yakvm_destroy_vmm(struct vmm *vmm)
{
    free_page((unsigned long)phys_to_virt(vmm->ncr3));
    kfree(vmm);
}
