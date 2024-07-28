#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>

#define PROGRAM_PATH "/path/to/the/userspace/binary"
#define INPUT_FILE "/path/to/output/file/of/userspace/file"

static int run_user_program(void)
{
    char *argv[] = { PROGRAM_PATH, NULL };
    char *envp[] = { "HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
    return call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
}

unsigned long sym_addr_lookup(void)
{
    struct file *f;
    char *buf;
    loff_t pos = 0;
    int ret;
    unsigned long sym_addr;

    ret = run_user_program();
    if (ret) {
        pr_err("Failed to run user program: %d\n", ret);
        return ret;
    }

    buf = kmalloc(32, GFP_KERNEL);
    if (!buf) {
        pr_err("Failed to allocate memory\n");
        return -ENOMEM;;
    }

    f = filp_open(INPUT_FILE, O_RDONLY, 0);
    if (IS_ERR(f)) {
        pr_err("Error opening file: %ld\n", PTR_ERR(f));
        kfree(buf);
        return PTR_ERR(f);
    }

    ret = kernel_read(f, buf, 32, & pos);
    filp_close(f, NULL);

    if (ret < 0) {
        pr_err("Error reading file: %d\n", ret);
        kfree(buf);
        return ret;
    }

    buf[ret] = '\0';
    kstrtoul(buf, 16, &sym_addr);
    pr_info("Address of sym: %lx\n", sym_addr);

    kfree(buf);
    return sym_addr;
}


static int __init lkm_init(void)
{
    unsigned long addr;
    addr = sym_addr_lookup();
    printk(KERN_INFO, "The address of read is: %lx\n", addr);
    return 0;
}

static void __exit lkm_exit(void)
{
    printk(KERN_INFO, "Bye\n");
}

MODULE_LICENSE("MIT");
MODULE_AUTHOR("cy83rc0llect0r");
MODULE_DESCRIPTION("read syscall address lookup");
MODULE_VERSION("1");

module_init(lkm_init);
module_exit(lkm_exit);
