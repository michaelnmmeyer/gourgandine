#include <stdlib.h>
#include <stdio.h>
#include "mem.h"

local noreturn void gn_fatal(const char *msg, ...)
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

local void *gn_malloc(size_t size)
{
   assert(size);
   void *mem = malloc(size);
   if (!mem)
      GN_OOM();
   return mem;
}

local void *gn_realloc(void *mem, size_t size)
{
   assert(size);
   mem = realloc(mem, size);
   if (!mem)
      GN_OOM();
   return mem;
}
