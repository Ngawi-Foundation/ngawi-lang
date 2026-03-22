#ifndef NGAWI_DIAG_H
#define NGAWI_DIAG_H

#include <stdarg.h>

void diag_error(const char *file, int line, int col, const char *fmt, ...);
void diag_note(const char *file, int line, int col, const char *fmt, ...);

void diag_error_source(const char *file,
                       const char *source,
                       int line,
                       int col,
                       const char *fmt,
                       ...);
void diag_note_source(const char *file,
                      const char *source,
                      int line,
                      int col,
                      const char *fmt,
                      ...);

#endif
