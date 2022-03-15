#include <stdarg.h>
#include <stdio.h>

#include "log.h"

FILE *log_file = NULL;

#define ENSURE_FP()                                                            \
  if (log_file == NULL) {                                                      \
    log_file = stdout;                                                         \
  }

void subst_log_file_set(const char *file_path) {
  if (log_file == NULL) {
    log_file = fopen(file_path, "w");
  }
}

void subst_log(const char *format, ...) {
  va_list args;

  ENSURE_FP();

  va_start(args, format);
  vfprintf(log_file, format, args);
  va_end(args);
  fflush(log_file);
}

void subst_log_mem(void *ptr, const char *format, ...) {
  va_list args;

  ENSURE_FP();

  // Prefix the line with the memory address
  fprintf(log_file, "%p: ", ptr);

  va_start(args, format);
  vfprintf(log_file, format, args);
  va_end(args);
  fflush(log_file);
}
