#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmd.h"
#include "../gourgandine.h"
#include "../src/vec.h"
#include "../src/lib/mascara.h"
#include "../src/lib/utf8proc.h"

#define MAX_FILE_SIZE (50 * 1024 * 1024)

static void *read_file(const char *path, size_t *size)
{
   FILE *fp = stdin;
   if (path) {
      fp = fopen(path, "r");
      if (!fp) {
         complain("cannot open '%s':", path);
         return NULL;
      }
   } else {
      path = "<stdin>";
   }

   uint8_t *buf = GN_VEC_INIT;
   size_t len = 0;
   
   gn_vec_grow(buf, BUFSIZ);
   while ((len = fread(&buf[gn_vec_len(buf)], 1, BUFSIZ, fp))) {
      gn_vec_len(buf) += len;
      if (gn_vec_len(buf) > MAX_FILE_SIZE) {
         complain("input file '%s' too large (limit is %d)", path, MAX_FILE_SIZE);
         goto fail;
      }
      gn_vec_grow(buf, BUFSIZ);
   }
   if (ferror(fp)) {
      complain("cannot read '%s':", path);
      goto fail;
   }
   if (fp != stdin)
      fclose(fp);

   uint8_t *nrm;
   ssize_t ret = utf8proc_map(buf, gn_vec_len(buf), &nrm, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
   if (ret < 0) {
      complain("cannot process file '%s': %s", path, utf8proc_errmsg(ret));
      goto fail;
   }

   gn_vec_free(buf);
   *size = ret;
   return nrm;

fail:
   gn_vec_free(buf);
   fclose(fp);
   return NULL;
}

noreturn static void version(void)
{
   const char *msg =
   "Gourgandine version "GN_VERSION"\n"
   "Copyright (c) 2016 Michaël Meyer"
   ;
   puts(msg);
   exit(EXIT_SUCCESS);
}

static void process(struct mascara *mr, struct gourgandine *gn, const char *path)
{
   size_t len;
   char *str = read_file(path, &len);
   if (!str)
      return;

   mr_set_text(mr, str, len);
   
   struct mr_token *sent;
   while ((len = mr_next(mr, &sent))) {
      struct gn_acronym def = {0};
      while (gn_search(gn, sent, len, &def))
         printf("%s\t%s\n", def.acronym, def.expansion);
   }
   free(str);
}

static void display_langs(void)
{
   const char *const *langs = mr_langs();
   while (*langs)
      puts(*langs++);
}

int main(int argc, char **argv)
{
   const char *lang = "en";
   bool list = false;
   struct option opts[] = {
      {'l', "lang", OPT_STR(lang)},
      {'L', "list", OPT_BOOL(list)},
      {'\0', "version", OPT_FUNC(version)},
      {0},
   };
   const char help[] =
      #include "gourgandine.ih"
   ;
   parse_options(opts, help, &argc, &argv);
   
   if (list) {
      display_langs();
      return EXIT_SUCCESS;
   }

   struct mascara *mr = mr_alloc(lang, MR_SENTENCE);
   if (!mr)
      die("no tokenizer for '%s'", lang);

   struct gourgandine *gn = gn_alloc();
   if (!argc) {
      process(mr, gn, NULL);
   } else {
      while (*argv)
         process(mr, gn, *argv++);
   }
   mr_dealloc(mr);
   gn_dealloc(gn);
}
