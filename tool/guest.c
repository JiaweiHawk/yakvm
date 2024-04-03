#include "guest.h"
#include "../include/cpu.h"

void entry(void)
{
        /* send 1 to *YAKVM_IO_HAWK* */
        out8(YAKVM_IO_HAWK, 1);

        /* expect to receive 2 from *YAKVM_IO_HAWK* */
        while(in8(YAKVM_IO_HAWK) != 2) {
                ;
        }

        hlt();
}
