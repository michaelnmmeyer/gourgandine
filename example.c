/* Extracts acronyms from a sentence given as argument on the command-line. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mascara.h"
#include "gourgandine.h"

int main(int argc, char **argv)
{
   if (argc != 2) {
      fprintf(stderr, "Usage: %s <sentence>\n", *argv);
      return EXIT_FAILURE;
   }

   /* Prepare ourselves for processing. */
   struct mascara *mr;
   int ret = mr_alloc(&mr, "en fsm", MR_SENTENCE);
   if (ret) {
      fprintf(stderr, "cannot create tokenizer: %s\n", mr_strerror(ret));
      return EXIT_FAILURE;
   }
   struct gourgandine *gn = gn_alloc();

   /* Tokenize the string at argv[1]. */
   struct mr_token *sent;
   size_t sent_len;
   mr_set_text(mr, argv[1], strlen(argv[1]));
   while ((sent_len = mr_next(mr, &sent))) {
      /* Iterate over all acronym definitions in this sentence. */
      struct gn_acronym acr = {0};
      while (gn_search(gn, sent, sent_len, &acr))
         printf("%s\t%s\n", acr.acronym, acr.expansion);
   }

   mr_dealloc(mr);
   gn_dealloc(gn);
}
