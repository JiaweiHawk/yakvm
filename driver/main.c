#include <linux/module.h>
#include "../include/yakvm.h"

/* information for module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hawkins Jiawei");

static int yakvm_init(void)
{
    log(LOG_INFO, "initialize yakvm");

    return 0;
}

static void yakvm_exit(void)
{
    log(LOG_INFO, "cleanup yakvm");
}

module_init(yakvm_init);
module_exit(yakvm_exit);
