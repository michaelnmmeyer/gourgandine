#ifndef GN_IMP_H
#define GN_IMP_H

#include <stdint.h>

struct gourgandine {
   char *buf;
   /* Temporary buffer for normalizing an acronym and its expansion.
      Before trying to match an acronym against its expansion, we write
      here a string of the form:

         acronym TAB (expansion_word SPACE)+

      Before calling the provided callback, we write here a string of the form:

         acronym '\0' expansion '\0'
    */
   
   int32_t *str;
   
   /* Mapping between a real token and a normalized token. */
   struct assoc {
      /* Offset in the above buffer of the current normalized token. */
      size_t norm_off;
      /* Position of the corresponding real token in the sentence. */
      size_t token_no;
   } *tokens;

   struct gn_acronym *acrs;   /* Acronyms gathered so far. */
};

void gn_run(struct gourgandine *rec, const struct mr_token *sent, size_t len);

#endif
