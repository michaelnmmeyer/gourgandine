#ifndef GN_BUF_H
#define GN_BUF_H

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

struct gn_buf {
   char *data;
   size_t size, alloc;
};

#define GN_BUF_INIT {.data = ""}

static inline void gn_buf_fini(struct gn_buf *buf)
{
   if (buf->alloc)
      free(buf->data);
}

void gn_buf_grow(struct gn_buf *buf, size_t incr);

/* Concatenates "data" at the end of a buffer. */
void gn_buf_cat(struct gn_buf *buf, const void *data, size_t size);

/* Same as above, for a character. */
static inline void gn_buf_catc(struct gn_buf *buf, int c)
{
   gn_buf_grow(buf, 1);
   buf->data[buf->size++] = c;
   buf->data[buf->size] = '\0';
}

/* Truncation to a given size. */
static inline void gn_buf_truncate(struct gn_buf *buf, size_t size)
{
   assert(size < buf->alloc || size == 0);
   if (buf->alloc)
      buf->data[buf->size = size] = '\0';
}

/* Truncation to the empty string. */
#define gn_buf_clear(buf) gn_buf_truncate(buf, 0)

#endif
