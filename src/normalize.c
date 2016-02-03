#include "imp.h"
#include "api.h"

static size_t norm_exp(char *buf, const char *str, size_t len)
{
   size_t new_len = 0;

   for (size_t i = 0; i < len; ) {
      int32_t c;
      i += gn_decode_char(&c, &str[i]);

      /* Skip quotation marks. */
      if (gn_is_double_quote(c))
         continue;
      /* Reduce whitespace spans to a single space character. */
      if (gn_is_space(c)) {
         do {
            /* Trailing spaces should already be removed. */
            assert(i < len);
            i += gn_decode_char(&c, &str[i]);
         } while (gn_is_space(c));
         buf[new_len++] = ' ';
      }
      new_len += gn_encode_char(&buf[new_len], c);
   }
   return new_len;
}

static size_t norm_abbr(char *buf, const char *str, size_t len)
{
   size_t new_len = 0;

   /* Drop internal periods. */
   for (size_t i = 0; i < len; i++)
      if (str[i] != '.')
         buf[new_len++] = str[i];
   return new_len;
}

local void gn_extract(struct gourgandine *rec, const struct mr_token *sent,
                      struct gn_acronym *def)
{
   size_t acr_len = sent[def->acronym_start].len;
   size_t exp_len = sent[def->expansion_end - 1].offset
                  + sent[def->expansion_end - 1].len
                  - sent[def->expansion_start].offset;

   gn_vec_clear(rec->buf);
   gn_vec_grow(rec->buf, exp_len + 1 + acr_len + 1);

   exp_len = norm_exp(rec->buf, sent[def->expansion_start].str, exp_len);
   def->expansion = rec->buf;
   def->expansion_len = exp_len;
   rec->buf[exp_len] = '\0';

   acr_len = norm_abbr(&rec->buf[exp_len + 1], sent[def->acronym_start].str, acr_len);
   def->acronym = &rec->buf[exp_len + 1];
   def->acronym_len = acr_len;
   rec->buf[exp_len + 1 + acr_len] = '\0';
}
