#include "devices.h"
#include "../include/cpu.h"

static uint8_t IO_HAWK = 0;

uint8_t yakvm_device_io_get(void)
{
        return IO_HAWK + 1;
}

void yakvm_device_io_set(uint8_t val)
{
        IO_HAWK = val;
}
