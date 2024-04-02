#ifndef __YAKVM_TOOL_GUEST_H_

        #define __YAKVM_TOOL_GUEST_H_

        #define hlt() \
                do { \
                        asm volatile("hlt"); \
                } while(0)

#endif
