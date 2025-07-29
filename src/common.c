#include "common.h"
#include <stdarg.h>
#include <stdio.h>

NORETURN void report_error(const char *source, uint32_t start, uint16_t len, const char *fmt, ...) {
  uint32_t line_start;
  uint32_t line_count = 1;
  uint32_t i;

  // Line count
  for (i = 0; i < start; ++i) if (source[i] == '\n') line_count++;
  // Line start
  for (i = start; i > 0 && source[i - 1] != '\n'; --i);
  line_start = i;
  // Line end
  for (i = start; source[i] != 0 && source[i] !=  '\n'; ++i);
  // Can't be multiline for now
  assert(i >= start + len);

  uint32_t number_len = 1;
  uint32_t value = line_count;
  while (value > 9) { number_len++; value /= 10; }

  // Print formatted message
  fprintf(stderr, ANSI_RED "\nERROR: " ANSI_RESET);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fprintf(stderr, "\n\n");
  fprintf(stderr, ANSI_BLUE " %d" ANSI_RESET " | ", line_count);
  fprintf(stderr, "%.*s", start - line_start, &source[line_start]);
  fprintf(stderr, ANSI_RED "%.*s" ANSI_RESET, len, &source[start]);
  fprintf(stderr, "%.*s\n    ", i - start - len, &source[start + len]);
  for (i = 0; i < start - line_start + number_len; ++i) fputc(' ', stderr);
  fprintf(stderr, ANSI_RED);
  for (i = 0; i < len; ++i) fputc('^', stderr);
  fprintf(stderr, ANSI_RESET "\n");
  exit(1);

}
