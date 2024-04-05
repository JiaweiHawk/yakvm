#ifndef __YAKVM_TOOL_DEVICES_H_

        #define __YAKVM_TOOL_DEVICES_H_

        #include <stdint.h>
        uint8_t yakvm_device_pio_get(void);
        void yakvm_device_pio_set(uint8_t val);

        uint8_t yakvm_device_mmio_get(void);
        void yakvm_device_mmio_set(uint8_t val);

#endif // __YAKVM_TOOL_DEVICES_H_
