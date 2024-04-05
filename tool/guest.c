#include "guest.h"
#include "../include/cpu.h"

void entry(void)
{
        /* send 1 to *YAKVM_PIO_HAWK* */
        pio_out8(YAKVM_PIO_HAWK, 1);
        /* expect to receive 2 from *YAKVM_PIO_HAWK* */
        while(pio_in8(YAKVM_PIO_HAWK) != 2) {
                ;
        }

        /* send 3 to *YAKVM_MMIO_HAWK* */
        mmio_out8(YAKVM_MMIO_HAWK, 3);
        /* expect to receive 2 from *YAKVM_MMIO_HAWK* */
        while(mmio_in8(YAKVM_MMIO_HAWK) != 2) {
                ;
        }

        hlt();
}
