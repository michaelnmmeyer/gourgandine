#include <stdio.h>
#include "buf.h"

#define MB_ENLARGE(buf, size, alloc, init) do {                                \
   const size_t size_ = (size);                                                \
   assert(size_ > 0 && init > 0);                                              \
   if (size_ > alloc) {                                                        \
      if (!alloc) {                                                            \
         alloc = size_ > init ? size_ : init;                                  \
         buf = malloc(alloc * sizeof *(buf));                                  \
      } else {                                                                 \
         assert(buf);                                                          \
         size_t tmp_ = alloc + (alloc >> 1);                                   \
         alloc = tmp_ > size_ ? tmp_ : size_;                                  \
         buf = realloc(buf, alloc * sizeof *(buf));                            \
      }                                                                        \
   }                                                                           \
} while (0)

void gn_buf_grow(struct gn_buf *buf, size_t size)
{
   size += buf->size;
   MB_ENLARGE(buf->data, size + 1, buf->alloc, 16);
}

void gn_buf_set(struct gn_buf *buf, const void *data, size_t size)
{
   gn_buf_clear(buf);
   gn_buf_cat(buf, data, size);
}

void gn_buf_cat(struct gn_buf *buf, const void *data, size_t size)
{
   gn_buf_grow(buf, size);
   memcpy(&buf->data[buf->size], data, size);
   buf->data[buf->size += size] = '\0';
}
