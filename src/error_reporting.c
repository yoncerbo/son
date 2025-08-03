#ifndef INCLUDE_ERROR_REPORTING
#define INCLUDE_ERROR_REPORTING

#include "common.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

void print_source_span(const char *source, uint32_t start, uint16_t len) {
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

  fprintf(stderr, ANSI_BLUE " %d" ANSI_RESET " | ", line_count);
  fprintf(stderr, "%.*s", start - line_start, &source[line_start]);
  fprintf(stderr, ANSI_RED "%.*s" ANSI_RESET, len, &source[start]);
  fprintf(stderr, "%.*s\n    ", i - start - len, &source[start + len]);
  for (i = 0; i < start - line_start + number_len; ++i) fputc(' ', stderr);
  fprintf(stderr, ANSI_RED);
  for (i = 0; i < len; ++i) fputc('^', stderr);
  fprintf(stderr, ANSI_RESET "\n");
}

void print_note(const char *source, uint32_t start, uint16_t len, const char *fmt, ...) {
  fprintf(stderr, ANSI_MAGENTA "NOTE: " ANSI_RESET);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n\n");
  print_source_span(source, start, len);
}

void vprint_error_message(const char *fmt, va_list args) {
  fprintf(stderr, ANSI_RED "\nERROR: " ANSI_RESET);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n\n");
}

void print_error_message(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprint_error_message(fmt, args);
  va_end(args);
}

void print_error(const char *source, uint32_t start, uint16_t len, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprint_error_message(fmt, args);
  print_source_span(source, start, len);
  va_end(args);
}

#endif
