#ifndef GN_VEC_H
#define GN_VEC_H

#include <stdlib.h>

extern size_t gn_vec_void[2];

#define GN_VEC_INIT (void *)&gn_vec_void[2]

#define gn_vec_header(vec) ((size_t *)((char *)(vec) - sizeof gn_vec_void))

#define gn_vec_len(vec)  gn_vec_header(vec)[0]
#define gn_vec_free(vec) gn_vec_free(gn_vec_header(vec))

#define gn_vec_grow(vec, nr) do {                                              \
   vec = gn_vec_grow(gn_vec_header(vec), nr, sizeof *(vec));                   \
} while (0)

#define gn_vec_clear(vec) do {                                                 \
   gn_vec_len(vec) = 0;                                                        \
} while (0)

#define gn_vec_push(vec, x) do {                                               \
   gn_vec_grow(vec, 1);                                                        \
   vec[gn_vec_len(vec)++] = x;                                                 \
} while (0)

void *(gn_vec_grow)(size_t *vec, size_t incr, size_t elt_size);

static inline void (gn_vec_free)(size_t *vec)
{
   if (vec != gn_vec_void)
      free(vec);
}

#endif
