#include "api.h"
#include "imp.h"
#include "mem.h"
#include "vec.h"
#include "buf.h"

struct gourgandine *gn_alloc(void)
{
   struct gourgandine *gn = gn_malloc(sizeof *gn);
   *gn = (struct gourgandine){
      .buf = GN_BUF_INIT,
      .str = GN_VEC_INIT,
      .acrs = GN_VEC_INIT,
      .tokens = GN_VEC_INIT,
   };
   return gn;
}

const struct gn_acronym *gn_process(struct gourgandine *gn,
                                    const struct mr_token *sent, size_t len,
                                    size_t *nr)
{
   gn_vec_clear(gn->acrs);
   gn_run(gn, sent, len);
   *nr = gn_vec_len(gn->acrs);
   return *nr ? gn->acrs : NULL;
}

void gn_dealloc(struct gourgandine *gn)
{
   gn_buf_fini(&gn->buf);
   gn_vec_free(gn->str);
   gn_vec_free(gn->acrs);
   gn_vec_free(gn->tokens);
   free(gn);
}
