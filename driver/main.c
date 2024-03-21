#include <asm/msr.h>
#include <asm-generic/errno.h>
#include <asm-generic/fcntl.h>
#include <asm/processor.h>
#include <linux/anon_inodes.h>
#include <linux/atomic/atomic-instrumented.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/types.h>
#include "../include/vm.h"
#include "../include/yakvm.h"

/* information for module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hawkins Jiawei");

#define YAKVM_INUSE     1
#define YAKVM_UNUSE     0
static atomic_t yakvm_status = ATOMIC_INIT(YAKVM_UNUSE);

/* create the vm and return the vm fd */
static int yakvm_dev_ioctl_create_vm(void)
{
        int r;
        struct vm *vm;

        vm = yakvm_create_vm();
        if (IS_ERR(vm)) {
                r = PTR_ERR(vm);
                log(LOG_ERR, "yakvm_create_vm() failed "
                    "with error code %d", r);
                return r;
        }

        r = anon_inode_getfd("kvm-vm", &yakvm_vm_fops, vm, O_RDWR);
        if (r < 0) {
                log(LOG_ERR, "anon_inode_getfile() failed "
                    "with error code %d", r);
                yakvm_destroy_vm (vm);
                return r;
        }

        return r;
}

static long yakvm_dev_ioctl(struct file *filp, unsigned int ioctl,
                            unsigned long arg)
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
 * "15.4" on page 499 at
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
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

/*
 * Before any SVM instruction can be used, *EFER.SVME* must be
 * set to 1 according to "15.4" on page 499 at
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */
static void yakvm_cpu_svm_enable(void *data)
{
        uint64_t efer;
        rdmsrl(MSR_EFER, efer);
        wrmsrl(MSR_EFER, efer | EFER_SVME);
}

static int yakvm_init(void)
{
        int ret;

        if (atomic_xchg(&yakvm_status, YAKVM_INUSE) == YAKVM_INUSE) {
                log(LOG_ERR, "yakvm is inuse");
                return -EBUSY;
        }

        /* check whether cpu support SVM */
        if (!is_svm_supported()) {
                log(LOG_ERR, "SVM is not supported on this platform");
                return -EOPNOTSUPP;
        }

        /* exposes "/dev/yakvm" device to userspace */
        ret = misc_register(&yakvm_dev);
        if (ret) {
                log(LOG_ERR, "misc_register() failed with error code %d",
                    ret);
                return ret;
        }

        /* enable svm on cpus */
        on_each_cpu(yakvm_cpu_svm_enable, NULL, 1);

        log(LOG_INFO, "initialize yakvm");
        return 0;
}

static void yakvm_exit(void)
{
        misc_deregister(&yakvm_dev);

        assert(atomic_xchg(&yakvm_status, YAKVM_UNUSE) == YAKVM_INUSE);

        log(LOG_INFO, "cleanup yakvm");
}

module_init(yakvm_init);
module_exit(yakvm_exit);
