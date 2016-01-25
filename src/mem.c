#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mem.h"

noreturn void gn_fatal(const char *msg, ...)
{
   va_list ap;

   fputs("gourgandine: ", stderr);
   va_start(ap, msg);
   vfprintf(stderr, msg, ap);
   va_end(ap);
   putc('\n', stderr);
   abort();
}

#define GN_OOM() gn_fatal("out of memory")

void *gn_malloc(size_t size)
{
   assert(size);
   void *mem = malloc(size);
   if (!mem)
      GN_OOM();
   return mem;
}

void *gn_calloc(size_t nmemb, size_t size)
{
   assert(size);
   void *mem = calloc(nmemb, size);
   if (!mem)
      GN_OOM();
   return mem;
}

void *gn_realloc(void *mem, size_t size)
{
   assert(size);
   mem = realloc(mem, size);
   if (!mem)
      GN_OOM();
   return mem;
}
