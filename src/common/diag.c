#include "diag.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void diag_vprint(const char *level,
                        const char *file,
                        int line,
                        int col,
                        const char *fmt,
                        va_list args) {
  fprintf(stderr, "%s:%d:%d: %s: ", file ? file : "<unknown>", line, col, level);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
}

static void print_source_snippet(const char *source, int target_line, int target_col) {
  if (!source || target_line <= 0 || target_col <= 0) return;

  const char *line_start = source;
  int line = 1;
  while (*line_start && line < target_line) {
    if (*line_start == '\n') line++;
    line_start++;
  }

  if (line != target_line) return;

  const char *line_end = line_start;
  while (*line_end && *line_end != '\n') line_end++;

  fwrite(line_start, 1, (size_t)(line_end - line_start), stderr);
  fputc('\n', stderr);

  int col = 1;
  const char *p = line_start;
  while (*p && p < line_end && col < target_col) {
    fputc(*p == '\t' ? '\t' : ' ', stderr);
    p++;
    col++;
  }

  fputc('^', stderr);
  fputc('\n', stderr);
}

void diag_error(const char *file, int line, int col, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  diag_vprint("error", file, line, col, fmt, args);
  va_end(args);
}

void diag_note(const char *file, int line, int col, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  diag_vprint("note", file, line, col, fmt, args);
  va_end(args);
}

void diag_error_source(const char *file,
                       const char *source,
                       int line,
                       int col,
                       const char *fmt,
                       ...) {
  va_list args;
  va_start(args, fmt);
  diag_vprint("error", file, line, col, fmt, args);
  va_end(args);
  print_source_snippet(source, line, col);
}

void diag_note_source(const char *file,
                      const char *source,
                      int line,
                      int col,
                      const char *fmt,
                      ...) {
  va_list args;
  va_start(args, fmt);
  diag_vprint("note", file, line, col, fmt, args);
  va_end(args);
  print_source_snippet(source, line, col);
}
