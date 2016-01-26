#ifndef GOURGANDINE_H
#define GOURGANDINE_H

#define GN_VERSION "0.2"

#include <stddef.h>

struct gourgandine *gn_alloc(void);
void gn_dealloc(struct gourgandine *);

/* An acronym definition. */
struct gn_acronym {

   /* The acronym and its expansion, normalized: periods are removed in the
    * acronym, while double quotes and excessive white space are trimmed in the
    * corresponding expansion. These strings do not point into the source
    * text. They are nul-terminated.
    */
   const char *acronym;
   size_t acronym_len;
   const char *expansion;
   size_t expansion_len;

   /* Start and end offset, in the input sentence, of the acronym and its
    * expansion. We count in tokens. The end offset of the expansion is the
    * offset of the token that follows its last word in the input sentence,
    * such that substracting the expansion start offset from it gives the
    * length, in tokens, of the expansion. The same applies to the abbreviation.
    */
   size_t acronym_start;
   size_t acronym_end;
   size_t expansion_start;
   size_t expansion_end;
};

/* Declared in mascara.h (https://github.com/michaelnmmeyer/mascara). */
struct mr_token;

/* Finds acronym definitions in a sentence.
 * If an acronym definition is found in the provided sentence, fills the
 * provided acronym structure with informations about it, and returns 1.
 * Otherwise, leaves the acronym structure untouched, and returns 0.
 * This must be called several times in a loop to obtain all acronyms in a
 * sentence. Before the first call, the acronym structure must be zeroed.
 * Afterwards, the same structure must be passed again, untouched: the offsets
 * it contains are used to determine where to restart on each call.
 */
int gn_search(struct gourgandine *, const struct mr_token *sent, size_t sent_len,
              struct gn_acronym *);

#endif
