#include <asm-generic/errno.h>
#include <linux/export.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include "../include/yakvm.h"

/* information for module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hawkins Jiawei");

static long yakvm_dev_ioctl(struct file *filp,
			  unsigned int ioctl, unsigned long arg)
{
    return -ENOSYS;
}

static struct file_operations yakvm_chardev_ops = {
	.unlocked_ioctl = yakvm_dev_ioctl,
};

/* interface for userspace-kernelspace interaction */
static struct miscdevice yakvm_dev = {
	MISC_DYNAMIC_MINOR,
	"yakvm",
	&yakvm_chardev_ops,
};

static int yakvm_init(void)
{
    int ret;

	/* exposes "/dev/yakvm" device to userspace */
	ret = misc_register(&yakvm_dev);
	if (ret) {
		log(LOG_ERR, "misc_register() failed with error code %d", ret);
        return ret;
	}

    log(LOG_INFO, "initialize yakvm");
    return 0;
}

static void yakvm_exit(void)
{
	misc_deregister(&yakvm_dev);

    log(LOG_INFO, "cleanup yakvm");
}

module_init(yakvm_init);
module_exit(yakvm_exit);
