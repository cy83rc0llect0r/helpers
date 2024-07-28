#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHM_NAME "/my_shm"
#define SHM_SIZE sizeof(unsigned long)


#define SYMBOL_NAME "__x64_sys_XXXXXXX" //e.g. __x64_sys_read
#define ADDR_LENGTH 16

unsigned long sym_addr_lookup() {
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
  return addr;
}

int main() {
  unsigned long address = sym_addr_lookup();
  int fd;
  unsigned long * shmaddr;

  // Create shared memory object
  fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open failed");
    return 1;
  }

  // Set the size of the shared memory object
  if (ftruncate(fd, SHM_SIZE) == -1) {
    perror("ftruncate failed");
    return 1;
  }

  // Map the shared memory object into the process's address space
  shmaddr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (shmaddr == MAP_FAILED) {
    perror("mmap failed");
    return 1;
  }

  // Store the address in shared memory
  * shmaddr = address;
  printf("Address 0x%lx stored in shared memory.\n", address);

  // Unmap the shared memory object
  munmap(shmaddr, SHM_SIZE);

  // Close the shared memory object
  close(fd);

  return 0;
}
