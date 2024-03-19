#include <asm/processor.h>
#include <asm-generic/errno.h>
#include <asm-generic/fcntl.h>
#include <linux/anon_inodes.h>
#include <linux/export.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/smp.h>
#include "../include/vm.h"
#include "../include/yakvm.h"

/* information for module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hawkins Jiawei");

#define YAKVM_MAX_FDNAME		12

/* create the vm and return the vm fd */
static int yakvm_dev_ioctl_create_vm(void)
{
	char fdname[YAKVM_MAX_FDNAME];
	int r, fd;
	struct vm *vm;
	struct file *file;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		log(LOG_ERR, "get_unused_fd_flags() failed "
			"with error code %d", fd);
		return fd;
	}

	snprintf(fdname, sizeof(fdname), "%d", fd);

	vm = yakvm_create_vm(fdname);
	if (IS_ERR(vm)) {
		r = PTR_ERR(vm);
		log(LOG_ERR, "yakvm_create_vm() failed "
			"with error code %d", r);
		goto put_fd;
	}

	file = anon_inode_getfile("kvm-vm", &yakvm_vm_fops, vm, O_RDWR);
	if (IS_ERR(file)) {
		r = PTR_ERR(file);
		log(LOG_ERR, "anon_inode_getfile() failed "
			"with error code %d", r);
		goto put_vm;
	}

	fd_install(fd, file);
	return fd;

put_vm:
	yakvm_put_vm(vm);
put_fd:
	put_unused_fd(fd);
	return r;
}

static long yakvm_dev_ioctl(struct file *filp,
			  unsigned int ioctl, unsigned long arg)
{
	int r;

	switch (ioctl) {
		case YAKVM_CREATE_VM:
			r = yakvm_dev_ioctl_create_vm();
			if (r < 0) {
				log(LOG_ERR, "yakvm_dev_ioctl_create_vm() "
					"failed with error code %d", r);
			}
			return r;

		default:
			log(LOG_ERR, "yakvm_dev_ioctl() get unknown ioctl %d",
				ioctl);
			return -EINVAL;
	}

	return 0;
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

/*
 * check whether cpu support SVM according to
 * "Appendix B" on page 117 at https://www.0x04.net/doc/amd/33047.pdf
 */
static bool is_svm_supported(void)
{
	int cpu = smp_processor_id();
	struct cpuinfo_x86 *c = &cpu_data(cpu);

	if (c->x86_vendor != X86_VENDOR_AMD) {
		log(LOG_ERR, "CPU %d is not AMD", cpu);
		return false;
	}

	if (!cpu_has(c, X86_FEATURE_SVM)) {
		log(LOG_ERR, "SVM not supported by CPU %d", cpu);
		return false;
	}

	return true;
}

static int yakvm_init(void)
{
    int ret;

	/* check whether cpu support SVM */
	if (!is_svm_supported()) {
		log(LOG_ERR, "SVM is not supported on this platform");
        return 0;
	}

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
