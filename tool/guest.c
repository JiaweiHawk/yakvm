#include "guest.h"

void entry(void)
{
        asm volatile(".zero 4");
}
