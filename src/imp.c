#include "lib/utf8proc.h"
#include "lib/mascara.h"
#include "api.h"
#include "utf8.h"
#include "buf.h"
#include "vec.h"
#include "mem.h"
#include "imp.h"

struct span {
   size_t start;
   size_t end;
};

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
static void push_letter(struct gn_buf *buf, char32_t c)
{
   assert((gn_char_class(c) & GN_ALPHA));
   
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
      gn_buf_catc(buf, 'o');
      break;
   case U'Æ': case U'æ':
      /* Never actually encountered this ligature. I think it is reasonable to
       * assume that it is used like Œ.
       */
      gn_buf_catc(buf, 'a');
      break;
   default: {
      /* The longest latin glyphs, after NFKC normalization, are the ligatures ﬃ
       * and ﬄ. Some glyphs of other scripts produce much wider sequences, but
       * are useless for our purpose, so we just ignore them.
       */
      int32_t cs[3];
      ssize_t len = utf8proc_decompose_char(c, cs, sizeof cs / sizeof *cs, norm_opts, NULL);
      if (len > 0 && len <= (ssize_t)(sizeof cs / sizeof *cs)) {
         gn_buf_grow(buf, len * sizeof(char32_t));
         for (ssize_t i = 0; i < len; i++)
            buf->size += utf8proc_encode_char(cs[i], (uint8_t *)&buf->data[buf->size]);
      }
   }
   }
}

static void clear_data(struct gourgandine *rec)
{
   gn_buf_clear(&rec->buf);
   gn_vec_clear(rec->tokens);
}

static void encode_abbr(struct gourgandine *rec, const struct span *abbr,
                        const struct mr_token *sent)
{
   assert(rec->buf.size == 0);

   for (size_t t = abbr->start; t < abbr->end; t++) {
      const struct mr_token *token = &sent[t];
      for (size_t i = 0; i < token->len; ) {
         char32_t c;
         i += mb_decode_char(&c, &token->str[i]);
         if ((gn_char_class(c) & GN_ALPHA))
            push_letter(&rec->buf, c);
      }
   }
   gn_buf_catc(&rec->buf, '\t');
}

static void encode_exp(struct gourgandine *rec, const struct span *exp,
                       const struct mr_token *sent)
{
   for (size_t t = exp->start; t < exp->end; t++) {
      bool in_token = false;
      const struct mr_token *token = &sent[t];
      for (size_t i = 0; i < token->len; ) {
         char32_t c;
         i += mb_decode_char(&c, &token->str[i]);
         if ((gn_char_class(c) & GN_ALPHA)) {
            if (!in_token) {
               in_token = true;
               struct assoc a = {
                  .norm_off = rec->buf.size,
                  .token_no = t,
               };
               gn_vec_push(rec->tokens, a);
            }
            push_letter(&rec->buf, c);
         } else if (in_token) {
            gn_buf_catc(&rec->buf, ' ');
            in_token = false;
         }
      }
      if (in_token)
         gn_buf_catc(&rec->buf, ' ');
   }
}

static char32_t char_at(const struct gourgandine *gn, size_t tok, size_t pos)
{
   return gn->buf.data[gn->tokens[tok].norm_off + pos];
}

/* Tries to match an acronym against a possible expansion.
 *
 * Previously we used the backwards matching method described by
 * [Schwartz & Hearst]. We add here the restriction that, for an acronym letter
 * to match one of the letters of the expansion, this letter must either occur
 * at the beginning of a word, or inside a word which first letter is matching.
 * Here we consider that a word is a sequence of alphabetic character.
 *
 * Adding this restriction helps to correct false matches of the kind:
 *
 *    [c]ompound wit[h] the formula (CH)
 *
 * Very few valid expansions don't conform to this pattern, e.g.:
 *
 *    [MARLANT]      Maritime Forces Atlantic
 *    [COMSUBLANT]   Commander Submarine Force, Atlantic Fleet
 *    [REFLEX]       REstitution de l'inFormation à L'EXpéditeur
 *
 * Returns the position of the normalized word containing the last acronym
 * letter incremented by one on success, or 0 on failure.
 */
static size_t match_here(struct gourgandine *rec, const char *abbr,
                         size_t tok, size_t pos)
{
   /* There is a match if we reached the end of the acronym. */
   if (*abbr == '\t')
      return tok + 1;
   
   assert(pos > 0);
   
   /* Try first to find the acronym letter in the current word. */
   char32_t c;
   size_t l;
   while ((c = char_at(rec, tok, pos)) != ' ') {
      if (c == *abbr && (l = match_here(rec, &abbr[1], tok, pos + 1)))
         return l;
      pos++;
   }
   
   /* Restrict the search to the first letter of one of the following words. */
   while (++tok < gn_vec_len(rec->tokens)) {
      if (char_at(rec, tok, 0) == *abbr && (l = match_here(rec, &abbr[1], tok, 1)))
         return l;
      /* Special treatment of the 'x'.
       *    AMS-IX   Amsterdam Internet Exchange
       *    PMX      Pacific Media Expo
       *    PBX      private branch exchange
       *    C.X.C    Caribbean Examinations Council
       *    IAX2     Inter-Asterisk eXchange
       */
      if (*abbr == 'x' && char_at(rec, tok, 1) == *abbr && (l = match_here(rec, &abbr[1], tok, 2)))
         return l;
   }
   return 0;
}

static bool extract_rev(struct gourgandine *rec, const struct mr_token *sent,
                        struct span *abbr, struct span *exp)
{
   clear_data(rec);
   
   encode_abbr(rec, abbr, sent);
   encode_exp(rec, exp, sent);

   gn_buf_truncate(&rec->buf, rec->buf.size);

   const char *str = rec->buf.data;
   size_t start = gn_vec_len(rec->tokens);
   while (start--) {
      if (char_at(rec, start, 0) == *str && match_here(rec, &str[1], start, 1)) {
         exp->start = rec->tokens[start].token_no;
         return true;
      }
   }
   return false;
}

/* It is often the case that an expansion between brackets is followed by
 * an explicit delimitor (quotation marks, etc.), and then by some other
 * information (typically the expansion translation, if the acronym
 * comes from a foreign language). The pattern is:
 *
 *    <acronym> (<expansion> <delimitor> <something>)
 *
 * If we can find an explicit delimitor, we truncate the expansion there.
 */
static void truncate_exp(const struct mr_token *sent, struct span *exp,
                         size_t end)
{
   /* We don't split the expansion on a comma if there was one before, because
    * in this case the expansion is likely to be an enumeration of items, of the
    * type:
    *
    *    GM&O (Gulf, Mobile and Ohio Railroad)
    */
   bool contains_comma = false;
   for (size_t i = exp->start; i < end; i++) {
      if (sent[i].len == 1 && *sent[i].str == ',') {
         contains_comma = true;
         break;
      }
   }

   for (size_t i = end; i < exp->end; i++) {
      if (sent[i].len > sizeof(char32_t))
         continue;
      char32_t c;
      if (mb_decode_char(&c, sent[i].str) != sent[i].len)
         continue;
      switch (c) {
      case U',':
         if (!contains_comma) {
            exp->end = i;
            return;
         }
         break;
      case U':': case U';': case U'-': case U'/':
      case U'"': case U'”': case U'»': case U'=': case U'(':
         exp->end = i;
         return;
      }
   }
}

static bool extract_fwd(struct gourgandine *rec, const struct mr_token *sent,
                        struct span *abbr, struct span *exp)
{
   clear_data(rec);
   
   encode_abbr(rec, abbr, sent);
   encode_exp(rec, exp, sent);

   gn_buf_truncate(&rec->buf, rec->buf.size);

   if (gn_vec_len(rec->tokens) == 0)
      return false;
   
   const char *str = rec->buf.data;
   if (*str != char_at(rec, 0, 0))
      return false;
   
   size_t end = match_here(rec, &str[1], 0, 1);
   if (!end)
      return false;
   
   /* Translate to an actual token offset. */
   end = rec->tokens[end - 1].token_no;
   if (end < exp->end)
      truncate_exp(sent, exp, end);

   return true;
}

static bool pre_check(const struct mr_token *acr)
{
   /* Require that 2 <= |acronym| <= 10.
    * Everybody uses these numbers, so we do that too.
    */
   size_t ulen = gn_utf8_len(acr->str, acr->len);
   if (ulen < 2 || ulen > 10)
      return false;
   
   /* Require that the acronym's first character is alphanumeric.
    * Everybody does that, too.
    */
   char32_t c;
   mb_decode_char(&c, acr->str);
   if (!(gn_char_class(c) & (GN_ALPHA | GN_DIGIT)))
      return false;
   
   /* Require that the acronym contains at least one capital letter if
    * |acronym| = 2, otherwise 2.
    * People generally require only one capital letter. By requiring 2 capital
    * letters when |acronym| > 2, we considerably reduce the probability
    * that a short capitalized common word, containing only one capital letter,
    * is mistaken for an acronym. We also miss acronyms by doing that,
    * but a quick check shows that these are almost always acronyms of
    * measure units (km., dl., etc.), which are not the most interesting anyway,
    * so this is a good tradeoff.  
    */
   size_t i = 0;
   while (i < acr->len) {
      i += mb_decode_char(&c, &acr->str[i]);
      if ((gn_char_class(c) & GN_UPPER)) {
         if (ulen == 2)
            return true;
         ulen = 2;
      }
   }
   return false;
}

static bool post_check(const struct mr_token *sent,
                       const struct span *abbr, const struct span *exp)
{
   const char *abbr_str = sent[abbr->start].str;
   size_t abbr_len = sent[abbr->end - 1].offset + sent[abbr->end - 1].len - sent[abbr->start].offset;

   const char *meaning = sent[exp->start].str;
   size_t meaning_len = sent[exp->end - 1].offset + sent[exp->end - 1].len - sent[exp->start].offset;

   /* Check the expansion length. Should not be too large. */
   size_t umlen = gn_utf8_len(meaning, meaning_len);
   if (umlen > 100)
      return false;
   
   /* Check if there are unmatched brackets. If this is the case, this indicates
    * that we read too far back in the string, and made the meaning start in a
    * text segment which doesn't belong to the current one, e.g.:
    *
    *    In Bangladesh operano il Communist Party of Bangla Desh
    *    (Marxist-Leninist) abbreviato in BSD (ML) e il Proletarian Party of
    *    Purba Bangla abbreviato in BPSP.
    *
    *    [ML] Marxist-Leninist) abbreviato in BSD
    *
    * This check can only be done after the meaning is extracted because the
    * meaning can very well include brackets, and we don't know in advance where
    * it begins. Examples of legit inner brackets:
    *
    *    [MtCO2e] Metric Tonne (ton) Carbon Dioxide Equivalent
    *    [CCP] critical control(s) point(s)
    *
    * The most common case, by far, is an unmatched closing bracket.
    */
   int depth = 0;
   for (size_t i = 0; i < meaning_len; i++) {
      if (meaning[i] == ')') {
         if (--depth < 0)
            return false;
      } else if (meaning[i] == '(') {
         depth++;
      }
   }
   if (depth)
      return false;
   
   /* Discard if len(abbr) / len(meaning) is not within a reasonable range.
    * Ratios were determined experimentally, using German texts, which tend
    * to include the highest one.
    */
   double ratio = gn_utf8_len(abbr_str, abbr_len) / (double)umlen;   
   if (ratio >= 1. || ratio <= 0.037)
      return false;
   
   /* Discard if the acronym is part of the meaning.
    * We do this check _after_ the acronym is extracted because the acronym
    * might very well occur elsewhere in the same sentence, but not be included
    * in the expansion.
    */
   for (size_t i = exp->start; i < exp->end; i++)
      if (sent[i].len == abbr_len && !memcmp(sent[i].str, abbr_str, abbr_len))
         return false;
   return true;
}

static void norm_exp(struct gn_buf *buf, const char *str, size_t len)
{
   gn_buf_grow(buf, len);
   
   for (size_t i = 0; i < len; ) {
      char32_t c;
      i += mb_decode_char(&c, &str[i]);
      gn_buf_grow(buf, 2 * sizeof(char32_t));
      
      /* Skip quotation marks. */
      if (c == U'"' || c == U'”' || c == U'“' || c == U'„' || c == U'«' || c == U'»')
         continue;
      /* Reduce whitespace spans to a single space character. */
      if ((gn_char_class(c) & GN_WHITESPACE)) {
         do {
            /* Trailing spaces should already be removed. */
            assert(i < len);
            i += mb_decode_char(&c, &str[i]);
         } while ((gn_char_class(c) & GN_WHITESPACE));
         buf->data[buf->size++] = ' ';
      }
      buf->size += mb_encode_char(&buf->data[buf->size], c);
   }
   gn_buf_truncate(buf, buf->size);
}

static void norm_abbr(struct gn_buf *buf, const char *str, size_t len)
{
   gn_buf_grow(buf, len);
   char *bufp = &buf->data[buf->size];
   
   /* Drop internal periods. */
   for (size_t i = 0; i < len; i++)
      if (str[i] != '.')
         *bufp++ = str[i];
   
   gn_buf_truncate(buf, bufp - buf->data);
}

static int save_acronym(struct gourgandine *gn,
                        const struct span *acr, const struct span *exp)
{
   struct gn_acronym def = {
      .acronym = acr->start,
      .expansion_start = exp->start,
      .expansion_end = exp->end,
   };
   gn_vec_push(gn->acrs, def);
   return 0;
}

void gn_extract(struct gourgandine *rec, const struct mr_token *sent, const struct gn_acronym *def,
                struct gn_str *acr, struct gn_str *exp)
{
   size_t exp_len = sent[def->expansion_end - 1].offset
                  + sent[def->expansion_end - 1].len
                  - sent[def->expansion_start].offset;

   gn_buf_clear(&rec->buf);
   norm_exp(&rec->buf, sent[def->expansion_start].str, exp_len);
   exp->str = rec->buf.data;
   exp->len = rec->buf.size;

   gn_buf_catc(&rec->buf, '\0');
   norm_abbr(&rec->buf, sent[def->acronym].str, sent[def->acronym].len);
   acr->str = &rec->buf.data[exp->len + 1];
   acr->len = rec->buf.size - exp->len - 1;
}

#define MAX_TOKENS 100

static void rtrim_sym(const struct mr_token *sent, struct span *str)
{
   while (str->end > str->start && sent[str->end - 1].type == MR_SYM)
      str->end--;
}

static void ltrim_sym(const struct mr_token *sent, struct span *str)
{
   while (str->start < str->end && sent[str->start].type == MR_SYM)
      str->start++;
}

static int examine_context(struct gourgandine *rec, const struct mr_token *sent,
                           struct span *exp, struct span *abbr)
{
   /* Drop uneeded symbols. We have the configuration:
    *
    *    <expansion> SYM* ( SYM* <abbreviation> SYM* )
    *
    * There is a problem with trimming quotation marks because there are cases
    * where they really shouldn't be, e.g.:
    *
    *    [ATUP] association « Témoignage d'un passé »
    *
    * But the proportion of extracted expansions ending with a non-alphanumeric
    * character is very small (1404 / 110321 = 0.013), so we don't bother to add
    * a special case for that. Furthermore, internal quotation marks are removed
    * when normalizing an expansion, so this doesn't matter.
    */
   rtrim_sym(sent, exp);
   ltrim_sym(sent, abbr);
   rtrim_sym(sent, abbr);
   
   /* Nothing to do if we end up with the empty string after truncation. */
   if (exp->start == exp->end || abbr->start == abbr->end)
      return 0;

   /* If the abbreviation would be too long, try the reverse form. */
   if (abbr->end - abbr->start > 1)
      goto reverse;

   /* Avoid pathological cases. // XXX maybe truncate here and also for the reverse form?*/
   if (exp->end - exp->start > MAX_TOKENS)
      exp->start = exp->end - MAX_TOKENS;
   
   /* Try the form <expansion> (<acronym>). */
   if (pre_check(&sent[abbr->start])) {
      /* Hearst requires that an expansion doesn't contain more tokens than:
       *
       *    min(|abbr| + 5, |abbr| * 2)
       *
       * I checked which pairs would be excluded by adding this restriction with
       * a Wikipedia French archive. This concerns 2216 / 40289 pairs. This
       * removes false positives, but appr. half the excluded pairs are valid.
       * This is because of the use of punctuation and sequences like
       * "et de l'", etc., which make the expansion longer e.g.:
       *
       *    DIRECCTE    Directeur régional des Entreprises, de la Concurrence,
       *                de la Consommation, du Travail et de l'Emploi
       *
       * I tried to tweak how the minimum length is computed to allow longer
       * expansions, but it turns out that the ratio correct_pair / wrong_pair
       * is not improved significantly by doing that. We could try something
       * more fine grained: not counting article or punctuation, etc., but I
       * don't think this would be beneficial overall, because invalid
       * pairs also contain articles and punctuation with, so it seems, the
       * same overall distribution.
       *
       * So we leave that restriction out. To filter out invalid pairs,
       * some better criteria should be used.
       */
      if (extract_rev(rec, sent, abbr, exp) && post_check(sent, abbr, exp))
         return save_acronym(rec, abbr, exp);
   }

reverse:
   /* Try the form <acronym> (<expansion>). We only look for an
    * acronym comprising a single token, but could also try with two token.
    */
   exp->start = exp->end - 1;
   if (pre_check(&sent[exp->start]) && extract_fwd(rec, sent, exp, abbr) && post_check(sent, exp, abbr))
      return save_acronym(rec, exp, abbr);
   return 0;
}

static size_t find_closing_bracket(const struct mr_token *sent,
                                   size_t pos, size_t len, int lb, int rb)
{
   int nest = 1;

   for ( ; pos < len; pos++) {
      if (sent[pos].len == 1) {
         if (*sent[pos].str == lb)
            nest++;
         else if (*sent[pos].str == rb && --nest <= 0)
            break;
      }
   }
   /* We allow unmatched opening brackets because it is still possible to match
    * the pattern:
    *
    *    <acronym> (<expansion> [missing ')']
    */
   return nest >= 0 ? pos : 0;
}

void gn_run(struct gourgandine *rec, const struct mr_token *sent, size_t len)
{
   struct span left, right;
   size_t lb, rb;
   
   left.start = 0;
   
   /* Start at 1 because there must be at least one token before the first
    * opening bracket. End at sent->size - 2 because the opening bracket must
    * be followed by at least one token and then a closing bracket.
    */
   for (size_t i = 1; i + 2 < len; i++) {
      if (sent[i].len != 1)
         continue;
      switch (*sent[i].str) {
      /* If the current token is an explicit delimitor, truncate the current
       * expansion on the left. Commas are not explicit delimitors because they
       * often appear in expansions.
       */
      case ';': case ':':
         left.start = i + 1;
         break;
      case '(': lb = '('; rb = ')'; goto bracket;
      case '[': lb = '['; rb = ']'; goto bracket;
      case '{': lb = '{'; rb = '}'; goto bracket;
      }
      continue;

   bracket:
      /* Find the corresponding closing bracket, allow nested brackets in the
       * interval.
       */
      right.end = find_closing_bracket(sent, i + 1, len, lb, rb);
      if (right.end) {
         left.end = i;
         right.start = i + 1;
         examine_context(rec, sent, &left, &right);
      }
   }
}
