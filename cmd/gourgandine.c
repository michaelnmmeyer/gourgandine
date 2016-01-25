#include <stdlib.h>
#include <stdio.h>
#include "cmd.h"
#include "../gourgandine.h"
#include "../src/lib/mascara.h"

#define MAX_FILE_SIZE (50 * 1024 * 1024)

static char *read_file(const char *path, size_t *sizep)
{
   FILE *fp = stdin;
   if (path) {
      fp = fopen(path, "r");
      if (!fp)
         die("cannot open '%s':", path);
   }

   char *data = malloc(MAX_FILE_SIZE); // FIXME later
   size_t size = fread(data, 1, MAX_FILE_SIZE, fp);

   if (ferror(fp))
      die("cannort read '%s':", path);
   if (size == MAX_FILE_SIZE && !feof(fp))
      die("input file too large (limit is %d)", MAX_FILE_SIZE);

   if (fp != stdin)
      fclose(fp);

   *sizep = size;
   return data;
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
   mr_set_text(mr, str, len);
   
   struct mr_token *sent;
   while ((len = mr_next(mr, &sent))) {
      size_t nr;
      const struct gn_acronym *defs = gn_process(gn, sent, len, &nr);
      for (size_t i = 0; i < nr; i++) {
         struct gn_str acr, exp;
         gn_extract(gn, sent, &defs[i], &acr, &exp);
         printf("%s\t%s\n", acr.str, exp.str);
      }
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
