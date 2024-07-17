#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kmod.h>

#define INPUT_FILE "/tmp/sym_addr"
#define PROGRAM_PATH "/path/to/userspace/file/reader/and/line/parser"

static unsigned long sym_addr = 0;

static int run_user_program(void) {
    char * argv[] = {
        PROGRAM_PATH,
        NULL
    };
    char * envp[] = {
        "HOME=/",
        "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
        NULL
    };
    return call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
}

static int __init my_module_init(void) {
    struct file * f;
    char * buf;
    loff_t pos = 0;
    int ret;

    // Run the user-space program
    ret = run_user_program();
    if (ret) {
        pr_err("Failed to run user program: %d\n", ret);
        return ret;
    }

    buf = kmalloc(32, GFP_KERNEL);
    if (!buf) {
        pr_err("Failed to allocate memory\n");
        return -ENOMEM;
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

    buf[ret] = '\0'; // Null-terminate the buffer
    kstrtoul(buf, 16, & sym_addr);
    pr_info("Address of %s: %lx\n", sym_addr);

    kfree(buf);
    return 0;
}

static void __exit my_module_exit(void) {
    pr_info("Module exiting. Address of sym was: %lx\n", sym_addr);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("AmirReza(cy83rc0llect0r)");
MODULE_DESCRIPTION("A module(helper) to find a sym address");
