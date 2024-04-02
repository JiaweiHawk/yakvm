#ifndef __YAKVM_MEMORY_H_

    #define __YAKVM_MEMORY_H_

    #define KiB * (1024)

    #include "yakvm.h"
    #ifdef __KERNEL__

        #include <asm/page_types.h>
        #define PTRS_PER_PAGE       (PAGE_SIZE / sizeof(unsigned long))
        typedef unsigned long entry;
        struct table {
            entry entrys[PTRS_PER_PAGE];
        };

        struct vmm {
            unsigned long ncr3;
            struct vm *vm;
        };

        /*
         * The long mode 4-Level page table layout is described
         * in "5.3" on page at 142
         * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf.
         */
        #define PT              1
        #define P_SHIFT         12
        #define PDT             2
        #define PD_SHIFT        21
        #define PDPT            3
        #define PDP_SHIFT       30
        #define PML4T           4
        #define PML4_SHIFT      39
        static inline uint32_t table_index(uint64_t addr, uint32_t level)
        {
            switch (level) {
                case PT:
                    return (addr >> P_SHIFT) & (PTRS_PER_PAGE - 1);
                case PDT:
                    return (addr >> PD_SHIFT) & (PTRS_PER_PAGE - 1);
                case PDPT:
                    return (addr >> PDP_SHIFT) & (PTRS_PER_PAGE - 1);
                case PML4T:
                    return (addr >> PML4_SHIFT) & (PTRS_PER_PAGE - 1);
            }
            /* never reach here */
            assert(false);
        }

        #include <asm/io.h>
        /* transfer physical base address to its page physical address */
        static inline unsigned long yakvm_vmm_page(unsigned long pa) {
            return pa & PAGE_MASK;
        }
        /* transfer physical base address to virtual address */
        static inline void *yakvm_vmm_phys_to_virt(unsigned long pa)
        {
            return phys_to_virt(yakvm_vmm_page(pa));
        }

        struct page *yakvm_vmm_npt_create(struct vmm *vmm,
                                          unsigned long gpa);
        struct vmm *yakvm_create_vmm(struct vm *vm);
        void yakvm_destroy_vmm(struct vmm *vmm);
    #else // __KERNEL__
        #define PAGE_SIZE       (4 KiB)
        #define PAGE_MASK       (~((unsigned long)PAGE_SIZE - 1))
    #endif

    static_assert(PAGE_SIZE == 4 KiB);

#endif // __YAKVM_MEMORY_H_
