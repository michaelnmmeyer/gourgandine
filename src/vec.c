#include <stdlib.h>
#include "vec.h"
#include "mem.h"

size_t gn_vec_void[2];

void *(gn_vec_grow)(size_t *vec, size_t nr, size_t elt_size)
{
   size_t need = vec[0] + nr;
   if (need < nr)
      gn_fatal("integer overflow");
   
   size_t alloc = vec[1];
   if (alloc >= need)
      return vec + 2;

   if (vec == gn_vec_void) {
      if (need > (SIZE_MAX - sizeof gn_vec_void) / elt_size)
         gn_fatal("integer overflow");
      vec = gn_malloc(sizeof gn_vec_void + need * elt_size);
      vec[0] = vec[1] = 0;
      return vec + 2;
   }
   
   alloc += (alloc >> 1);
   if (alloc < need)
      alloc = need;
   if (alloc > (SIZE_MAX - sizeof gn_vec_void) / elt_size)
      gn_fatal("integer overflow");
   vec = gn_realloc(vec, sizeof gn_vec_void + alloc * elt_size);
   vec[1] = alloc;
   return vec + 2;
}
