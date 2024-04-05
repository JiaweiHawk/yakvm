#include "devices.h"
#include "../include/cpu.h"

static uint8_t PIO_HAWK = 0;
uint8_t yakvm_device_pio_get(void)
{
        return PIO_HAWK + 1;
}
void yakvm_device_pio_set(uint8_t val)
{
        PIO_HAWK = val;
}

static uint8_t MMIO_HAWK = 0;
uint8_t yakvm_device_mmio_get(void)
{
        return MMIO_HAWK - 1;
}

void yakvm_device_mmio_set(uint8_t val)
{
        MMIO_HAWK = val;
}
