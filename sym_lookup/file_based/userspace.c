#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOL_NAME "__x64_sys_XXX" //e.g. __x64_sys_read
#define OUTPUT_FILE "/tmp/sym_addr.txt"
#define ADDR_LENGTH 16

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

  if (addr == 0) {
    fprintf(stderr, "Symbol %s not found\n", SYMBOL_NAME);
    return 1;
  }

  file = fopen(OUTPUT_FILE, "w");
  if (file == NULL) {
    perror("fopen");
    return 1;
  }

  fprintf(file, "%lx\n", addr);
  fclose(file);

  printf("Address of %s: %lx\n", SYMBOL_NAME, addr);
  return 0;
}
