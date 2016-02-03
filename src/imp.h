#ifndef GN_IMP_H
#define GN_IMP_H

#define local static

#include <stdint.h>

struct span {
   size_t start;
   size_t end;
};

struct gourgandine {

   /* Buffer for normalizing an acronym and its expansion. They are stored
    * consecutively: acronym '\0' expansion '\0'.
    */
   char *buf;

   /* Buffer for holding the string to match, which is normalized. We write here
    * a string of the form: acronym TAB (expansion_word SPACE)+.
    */
   int32_t *str;

   /* Over-segmenting tokens is necessary for matching, e.g.:
    *
    *    [GAP] D-glyercaldehyde 3-phosphate
    *
    * Our tokenizer doesn't split on '-', in particular, so we must perform a
    * new segmentation of each token. The following keeps track of the relation
    * between the token chunks we produce here and the position of the
    * corresponding token in the input sentence, so that we can obtain correct
    * offsets after processing.
    */
   struct assoc {
      /* Offset in the "str" of the current normalized token. */
      size_t norm_off;
      /* Position of the corresponding real token in the sentence. */
      size_t token_no;
   } *tokens;
};

struct gn_acronym;

local void gn_encode(struct gourgandine *rec, const struct mr_token *sent,
                     size_t abbr, const struct span *exp);

local void gn_extract(struct gourgandine *rec, const struct mr_token *sent,
                      struct gn_acronym *def);
#endif
