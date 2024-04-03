#ifndef __YAKVM_TOOL_GUEST_H_

        #define __YAKVM_TOOL_GUEST_H_

        /*
         * To build 16bit real mode code according to
         * http://dc0d32.blogspot.com/2010/06/real-mode-in-c-with-gcc-writing.html
         */
        __asm__(".code16gcc\n");

        static inline __attribute__((always_inline)) void hlt(void)
        {
                asm volatile("hlt");
        }

        #include <stdint.h>
        static inline __attribute__((always_inline))
        void out8(uint16_t port, uint8_t val)
        {
                asm volatile(
                        "outb %0, %1\n\t"
                        :
                        : "a"(val), "d"(port));
        }

        static inline __attribute__((always_inline))
        uint8_t in8(uint16_t port)
        {
                uint8_t val;
                asm volatile(
                        "inb %1, %0\n\t"
                        : "=a"(val)
                        : "d"(port));
                return val;
        }

#endif
