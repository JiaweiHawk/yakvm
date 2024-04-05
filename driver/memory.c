#include <asm/pgtable_types.h>
#include <asm/io.h>
#include <asm-generic/errno-base.h>
#include <linux/pfn_t.h>
#include <linux/pgtable.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/gfp_types.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "../include/memory.h"
#include "../include/yakvm.h"

/* create the pte for @gpa */
struct page *yakvm_vmm_npt_create(struct vmm *vmm, unsigned long gpa,
                                  bool is_mmio)
{
    struct table *table;
    struct page *page;
    int r, index;
    /*
     * the cr3 layout is described in "5.3.2" on page 140 at
     * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
     */
    unsigned long entry = vmm->ncr3;

    for (int level = PML4T; level >= PT; --level) {
        index = table_index(gpa, level);
        table = yakvm_vmm_phys_to_virt(entry);
        entry = table->entrys[index];
        if (!entry) {
            /* create *level* entry if needed */
            page = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
            if (!page) {
                log(LOG_ERR, "alloc_page() failed");
                r = -ENOMEM;
                goto out;
            }

            /*
             * entry layout is described in "5.3.3" on page 144 at
             * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
             *
             * page-translation-table entry field is described
             * in "5.4.1" on page 153 at
             * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
             *
             * The page must be writable by user at the nested page table
             * level according to "15.25.5" on page 550 at
             * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
             */
            entry = page_to_phys(page) | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER;
            table->entrys[index] = entry;
        } else if (level == PT) {
            r = -EEXIST;
        }
        assert((entry & _PAGE_PRESENT) && (entry & _PAGE_RW) && (entry & _PAGE_USER));
    }

    if (is_mmio) {
	    /*
	     * remove *_PAGE_PRESENT* bit to trigger a *NPF*
         * with exitinfo1.p to intercept the mmio
         * operations according to "15.25.6" on page 551 at
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
	     */
        table->entrys[index] &= ~(_PAGE_PRESENT);
    }

    return pfn_to_page(entry >> PAGE_SHIFT);

out:
    return ERR_PTR(r);
}

struct vmm *yakvm_create_vmm(struct vm *vm)
{
    int r;
    struct vmm *vmm;
    struct page *pml4t;
    struct page *mmio;

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

    mmio = yakvm_vmm_npt_create(vmm, YAKVM_MMIO_HAWK, true);
    if (IS_ERR(mmio)) {
        r = PTR_ERR(mmio);
        log(LOG_ERR, "yakvm_vmm_npt_create() "
            "failed with error code %x", r);
        goto free_vmm;
    }

    return vmm;

free_vmm:
    kfree(vmm);
free_pml4:
    __free_page(pml4t);
out:
    return ERR_PTR(r);
}

static void yakvm_vmm_destroy_table(struct table *table, int level)
{
    for (int idx = 0; idx < PTRS_PER_PAGE; ++idx) {
        unsigned long entry = table->entrys[idx];
        if (entry) {
            assert((entry & _PAGE_USER) && (entry & _PAGE_RW));
            if (level == PT) {
                free_page((unsigned long)yakvm_vmm_phys_to_virt(entry));
            } else {
                yakvm_vmm_destroy_table(yakvm_vmm_phys_to_virt(entry),
                                        level - 1);
            }
        }
    }
    free_page((unsigned long)table);
}

void yakvm_destroy_vmm(struct vmm *vmm)
{
    yakvm_vmm_destroy_table(yakvm_vmm_phys_to_virt(vmm->ncr3), PML4T);
    kfree(vmm);
}
