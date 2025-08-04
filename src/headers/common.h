#ifndef INCLUDE_COMMON
#define INCLUDE_COMMON

#include <stdint.h>

typedef struct {
  const char *ptr;
  uint32_t len;
} Str;

#define UNREACHABLE __builtin_unreachable();
#define NORETURN __attribute__((noreturn))

#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x) 
#define CSTR_LEN(str) (sizeof(str) - 1)
#define STR(str) ((Str){ (str), CSTR_LEN(str) })

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define LIMIT_UP(a, b) MIN(a, b)
#define LIMIT_DOWN(a, b) MAX(a, b)

#define DEBUGD(var) printf(STRINGIFY(var) "=%d\n", var)
#define DEBUGC(var) printf(STRINGIFY(var) "='%c'\n", var)
#define DEBUGS(var) printf(STRINGIFY(var) "=\"%s\"\n", var)
#define DEBUGX(var) printf(STRINGIFY(var) "=0x%x\n", var)

#define LOG(fmt, ...) \
  printf("[LOG] %s " __FILE__ ":" STRINGIFY(__LINE__) " " fmt, __func__, ##__VA_ARGS__)


// error_reporting.c
void print_source_span(const char *source, uint32_t start, uint16_t len);
void print_note(const char *source, uint32_t start, uint16_t len, const char *fmt, ...);
void print_error_message(const char *fmt, ...);
void print_error(const char *source, uint32_t start, uint16_t len, const char *fmt, ...);

// fs.c
const char *map_file_readonly(const char *filename);

// graphviz.c
#include "parser.h"
void output_graphviz_file(const char *filename, const Parser *p);

#endif

