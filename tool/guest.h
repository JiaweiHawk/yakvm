#ifndef __YAKVM_TOOL_GUEST_H_

        #define __YAKVM_TOOL_GUEST_H_

        /*
         * To build 16bit real mode code according to
         * http://dc0d32.blogspot.com/2010/06/real-mode-in-c-with-gcc-writing.html
         */
        __asm__(".code16gcc\n");

        #define hlt() \
                do { \
                        asm volatile("hlt"); \
                } while(0)

#endif
