#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <ctype.h>

#define NETLINK_USER 31
#define SYMBOL_NAME "__x64_sys_XXX"  //e.g. __x64_sys_read or __x64_sys_write
#define ADDR_LENGTH 16

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr * nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

int main() {

  FILE * file;
  char line[256];
  unsigned long addr = 0;
  char address[ADDR_LENGTH + 1];
  int found = 0;

  file = fopen("/proc/kallsyms", "r");
  if (file == NULL) {
    perror("fopen");
    return 1;
  }

  while (fgets(line, sizeof(line), file)) {
    char * pos = strstr(line, SYMBOL_NAME);
    if (pos && (pos == line + ADDR_LENGTH + 3)) {
      // Ensure exact match by checking boundaries
      if ((pos == line + ADDR_LENGTH + 3 || !isalnum( * (pos - 1))) && !isalnum( * (pos + strlen(SYMBOL_NAME)))) {
        strncpy(address, line, ADDR_LENGTH);
        address[ADDR_LENGTH] = '\0';
        found = 1;
        sscanf(address, "%lx", &addr);
        break;
      }
    }
  }
  fclose(file);
  // Create a Netlink socket
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if (sock_fd < 0) {
    perror("socket creation failed");
    return -1;
  }

  memset( & src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); // self pid

  bind(sock_fd, (struct sockaddr * ) & src_addr, sizeof(src_addr));

  memset( & dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; // For Linux Kernel
  dest_addr.nl_groups = 0; // unicast

  nlh = (struct nlmsghdr * ) malloc(NLMSG_SPACE(sizeof(unsigned long)));
  memset(nlh, 0, NLMSG_SPACE(sizeof(unsigned long)));
  nlh -> nlmsg_len = NLMSG_SPACE(sizeof(unsigned long));
  nlh -> nlmsg_pid = getpid();
  nlh -> nlmsg_flags = 0;

  memcpy(NLMSG_DATA(nlh), & addr, sizeof(unsigned long));

  iov.iov_base = (void * ) nlh;
  iov.iov_len = nlh -> nlmsg_len;
  msg.msg_name = (void * ) & dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = & iov;
  msg.msg_iovlen = 1;

  printf("Sending address to kernel: 0x%lx\n", addr);
  sendmsg(sock_fd, & msg, 0);

  close(sock_fd);
  return 0;
}
