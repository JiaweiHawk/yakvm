#ifndef __YAKVM_H_

    #define __YAKVM_H_

    /* log for kernel and user */
    #define LOG_INFO "\033[0;32m"
    #define LOG_ERR "\033[0;31m"
    #define LOG_NONE "\033[0m"

    #ifdef __KERNEL__
        #include <linux/printk.h>
        #include <linux/kern_levels.h>
        #define log(level, args...) do { \
            printk(KERN_DEFAULT level); \
            printk(KERN_CONT "[yakvm(%s:%d)]: ", __FILE__, __LINE__); \
            printk(KERN_CONT args); \
            printk(KERN_CONT LOG_NONE "\n"); \
        } while(0)
    #else // __KERNEL__
        #include <stdio.h>
        #define log(level, args...) do { \
            printf(level); \
            printf("[emulator(%s:%d)]: ", __FILE__, __LINE__); \
            printf(args); \
            printf(LOG_NONE "\n"); \
        } while(0)
    #endif // __KERNEL__

#endif // __YAKVM_H_
