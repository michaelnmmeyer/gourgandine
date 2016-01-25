#ifndef GN_MEM_H
#define GN_MEM_H

#include <stddef.h>
#include <stdarg.h>
#include <stdnoreturn.h>

noreturn void gn_fatal(const char *msg, ...);

void *gn_malloc(size_t)
#ifdef ___GNUC__
   __attribute__((malloc))
#endif
   ;

void *gn_realloc(void *, size_t);

#endif
