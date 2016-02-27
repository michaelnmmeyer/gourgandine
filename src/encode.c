#include <assert.h>
#include "lib/utf8proc.h"
#include "lib/mascara.h"
#include "imp.h"
#include "utf8.h"
#include "vec.h"

/* Before comparing an acronym to its expansion, we do the following:
 * (a) Use Unicode decomposition mappings (NFKC).
 * (b) Remove all non-alphabetic characters, including numbers. Removing numbers
 *     is necessary for matching acronyms like [NaH2PO4] against
 *     Natriumdihydrogenphosphat. Not trying to match numbers in the
 *     acronym against the expansion doesn't seem to have a negative effect
 *     on precision.
 * (c) Convert the text to lowercase. Casefolding seems not mandatory for our
 *     purpose, but we do it nonetheless.
 * (d) Remove diacritics. This is necessary because there is often a mismatch in
 *     the use of diacritics between the acronym and its expansion (the
 *     most common case being the acronym lacking diacritics).
 */
static const int norm_opts = UTF8PROC_COMPOSE | UTF8PROC_CASEFOLD
                           | UTF8PROC_STRIPMARK | UTF8PROC_COMPAT
                           ;

/* Fold a character to ASCII and appends it to the provided buffer. */
static int32_t *push_letter(int32_t *str, int32_t c)
{
   assert(kb_is_letter(c));

   switch (c) {
   case U'œ': case U'Œ':
      /* The ligature Œ requires a specific treatment:
       *    MOI      main-d’œuvre immigrée
       *    ETO      échographie trans-œsophagienne
       *    IOR      Institut pour les œuvres de religion
       *    HMONP    Habilitation à la Maîtrise d'Œuvre en son Nom Propre
       *    TOB      Traduction œcuménique de la Bible
       *    HADOPI   Haute autorité pour la diffusion des œuvres et la
       *             protection des droits sur internet
       */
      gn_vec_push(str, 'o');
      break;
   case U'Æ': case U'æ':
      /* Never actually encountered this ligature. I think it is reasonable to
       * assume that it is used like Œ.
       */
      gn_vec_push(str, 'a');
      break;
   default: {
      /* The longest latin glyphs, after NFKC normalization, are the ligatures ﬃ
       * and ﬄ. Some glyphs of other scripts produce much wider sequences, but
       * are useless for our purpose, so we just ignore them. Note that we don't
       * necessarily obtain a NFKC-normalized string: further processing is
       * required to do things properly (see utf8proc_decompose()), but we only
       * care about alphabetic characters encoded in a single code point, so
       * this doesn't matter.
       */
      int32_t cs[3];
      const size_t max = sizeof cs / sizeof *cs;
      ssize_t len = utf8proc_decompose_char(c, cs, max, norm_opts, NULL);
      if (len > 0 && (size_t)len <= max)
         for (ssize_t i = 0; i < len; i++)
            gn_vec_push(str, cs[i]);
   }
   }
   return str;
}

static void encode_abbr(struct gourgandine *rec, const struct mr_token *acr)
{
   assert(gn_vec_len(rec->str) == 0);

   for (size_t i = 0, clen; i < acr->len; i += clen) {
      char32_t c = kb_decode(&acr->str[i], &clen);
      if (kb_is_letter(c))
         rec->str = push_letter(rec->str, c);
   }
}

static void encode_exp(struct gourgandine *rec, const struct span *exp,
                       const struct mr_token *sent)
{
   for (size_t t = exp->start; t < exp->end; t++) {
      bool in_token = false;
      const struct mr_token *token = &sent[t];
      for (size_t i = 0, clen; i < token->len; i += clen) {
         char32_t c = kb_decode(&token->str[i], &clen);
         if (kb_is_letter(c)) {
            if (!in_token) {
               in_token = true;
               struct assoc a = {
                  .norm_off = gn_vec_len(rec->str),
                  .token_no = t,
               };
               gn_vec_push(rec->tokens, a);
            }
            rec->str = push_letter(rec->str, c);
         } else if (in_token) {
            gn_vec_push(rec->str, ' ');
            in_token = false;
         }
      }
      if (in_token)
         gn_vec_push(rec->str, ' ');
   }
}

local void gn_encode(struct gourgandine *rec, const struct mr_token *sent,
                     size_t abbr, const struct span *exp)
{
   gn_vec_clear(rec->str);
   gn_vec_clear(rec->tokens);

   encode_abbr(rec, &sent[abbr]);
   gn_vec_push(rec->str, '\t');
   encode_exp(rec, exp, sent);
   gn_vec_grow(rec->str, 1);
   rec->str[gn_vec_len(rec->str)] = '\0';
}
