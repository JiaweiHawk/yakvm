#include "guest.h"
#include "../include/cpu.h"

void entry(void)
{
        /* send 1 to *YAKVM_PIO_HAWK* */
        out8(YAKVM_PIO_HAWK, 1);

        /* expect to receive 2 from *YAKVM_PIO_HAWK* */
        while(in8(YAKVM_PIO_HAWK) != 2) {
                ;
        }

        hlt();
}
