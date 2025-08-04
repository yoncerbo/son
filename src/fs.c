#include "common.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

// Returns NULL on failure
const char *map_file_readonly(const char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) return NULL;

  struct stat st;
  if (fstat(fd, &st) < 0) return NULL;

  const char *file = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file == MAP_FAILED) return NULL;

  close(fd);
  return file;
}
