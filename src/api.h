#ifndef GOURGANDINE_H
#define GOURGANDINE_H

#define GN_VERSION "0.2"

#include <stddef.h>

struct gourgandine *gn_alloc(void);
void gn_dealloc(struct gourgandine *);

/* An acronym definition. */
struct gn_acronym {

   /* Offset of the acronym in the sentence, counting in tokens. */
   size_t acronym;

   /* Start and end offset of the corresponding expansion, also counted in
    * tokens. The expansion end offset is the offset of the token that follows
    * the last word in the expansion, such that substracting from it the start
    * index of the expansion gives the expansion length in tokens.
    */
   size_t expansion_start;
   size_t expansion_end;
};

/* Declared in mascara.h (https://github.com/michaelnmmeyer/mascara). */
struct mr_token;

/* Process a single sentence.
 * Fills *nr with the number of acronyms found. Returns an array of acronym
 * definitions, or NULL if none was found.
 */
const struct gn_acronym *gn_process(struct gourgandine *,
                                    const struct mr_token *sent, size_t len,
                                    size_t *nr);


struct gn_str {
   const char *str;
   size_t len;
};

void gn_extract(struct gourgandine *, const struct mr_token *sent, const struct gn_acronym *,
                struct gn_str *acr, struct gn_str *def);

#endif
