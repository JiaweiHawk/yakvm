#ifndef __YAKVM_MEMORY_H_

    #define __YAKVM_MEMORY_H_

    #define KiB * (1024)
    #define YAKVM_MEMORY        (32 KiB)

    #ifdef __KERNEL__
        #include <asm/page_types.h>
        static_assert(PAGE_SIZE == 4 KiB);

        #define PTRS_PER_PAGE       (PAGE_SIZE / sizeof(unsigned long))
        static_assert(PTRS_PER_PAGE == 512);

        /* Page Table */
        typedef unsigned long pte;
        struct pt {
            pte entrys[PTRS_PER_PAGE];
        };
        static_assert(sizeof(struct pt) == PAGE_SIZE);

        /* Page Directory Table */
        typedef unsigned long pdte;
        struct pdt {
            pdte entrys[PTRS_PER_PAGE];
        };
        static_assert(sizeof(struct pdt) == PAGE_SIZE);

        /* Page Directory Pointer Table */
        typedef unsigned long pdpte;
        struct pdpt {
            pdpte entrys[PTRS_PER_PAGE];
        };
        static_assert(sizeof(struct pdpt) == PAGE_SIZE);

        /* Page-Map Level-4 Table */
        typedef unsigned long pml4te;
        struct pml4t {
            pml4te entrys[PTRS_PER_PAGE];
        };
        static_assert(sizeof(struct pml4t) == PAGE_SIZE);

        struct vmm {
            unsigned long ncr3;
            struct vm *vm;
        };

        struct vmm *yakvm_create_vmm(struct vm *vm);
        void yakvm_destroy_vmm(struct vmm *vmm);
    #endif

#endif // __YAKVM_MEMORY_H_
