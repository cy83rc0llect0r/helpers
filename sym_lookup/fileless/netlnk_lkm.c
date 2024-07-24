#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netlink.h>
#include <net/sock.h>

#define NETLINK_USER 31

struct sock * nl_sk = NULL;

static void nl_recv_msg(struct sk_buff * skb) {
  struct nlmsghdr * nlh;
  unsigned long addr;

  nlh = (struct nlmsghdr * ) skb -> data;
  addr = * ((unsigned long * ) nlmsg_data(nlh));
  printk(KERN_INFO "Received address from user space: 0x%lx\n", addr);
}

static int __init netlnk_init(void) {
  struct netlink_kernel_cfg cfg = {
    .input = nl_recv_msg,
  };

  printk(KERN_INFO "Initializing Netlink socket\n");
  nl_sk = netlink_kernel_create( & init_net, NETLINK_USER, & cfg);
  if (!nl_sk) {
    printk(KERN_ALERT "Error creating Netlink socket\n");
    return -ENOMEM;
  }

  return 0;
}

static void __exit netlnk_exit(void) {
  printk(KERN_INFO "Exiting Netlink socket\n");
  netlink_kernel_release(nl_sk);
}

module_init(netlnk_init);
module_exit(netlnk_exit);
