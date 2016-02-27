#include "lib/utf8proc.h"
#include "lib/mascara.h"
#include "api.h"
#include "utf8.h"
#include "vec.h"
#include "mem.h"
#include "imp.h"
#include <string.h>
#include <assert.h>

static int32_t char_at(const struct gourgandine *gn, size_t tok, size_t pos)
{
   return gn->str[gn->tokens[tok].norm_off + pos];
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
static size_t match_here(struct gourgandine *rec, const int32_t *abbr,
                         size_t tok, size_t pos)
{
   /* There is a match if we reached the end of the acronym. */
   if (*abbr == '\t')
      return tok + 1;

   assert(pos > 0);

   /* Try first to find the acronym letter in the current word. */
   int32_t c;
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
                        size_t abbr, struct span *exp)
{
   gn_encode(rec, sent, abbr, exp);

   const int32_t *str = rec->str;
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
      if (sent[i].type != MR_SYM)
         continue;
      if (contains_comma && sent[i].len == 1 && *sent[i].str == ',')
         continue;
      exp->end = i;
      return;
   }
}

static bool extract_fwd(struct gourgandine *rec, const struct mr_token *sent,
                        size_t abbr, struct span *exp)
{
   gn_encode(rec, sent, abbr, exp);

   if (gn_vec_len(rec->tokens) == 0)
      return false;

   if (*rec->str != char_at(rec, 0, 0))
      return false;

   size_t end = match_here(rec, &rec->str[1], 0, 1);
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
   size_t ulen = kb_count(acr->str, acr->len);
   if (ulen < 2 || ulen > 10)
      return false;

   /* Require that the acronym's first character is alphanumeric.
    * Everybody does that, too.
    */
   size_t clen;
   char32_t c = kb_decode(acr->str, &clen);
   if (!gn_is_alnum(c))
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
   for (size_t i = 0; i < acr->len; i += clen) {
      c = kb_decode(&acr->str[i], &clen);
      if (kb_is_upper(c)) {
         if (ulen == 2)
            return true;
         ulen = 2;
      }
   }
   return false;
}

static bool post_check(const struct mr_token *sent,
                       size_t abbr, const struct span *exp)
{
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
   int nest = 0;
   for (size_t i = exp->start; i < exp->end; i++) {
      if (sent[i].len != 1)
         continue;
      if (*sent[i].str == ')' && --nest < 0)
         break;
      else if (*sent[i].str == '(')
         nest++;
   }
   if (nest)
      return false;

   /* Discard if the acronym is part of the expansion.
    * We do this check _after_ the acronym is extracted because the acronym
    * might very well occur elsewhere in the same sentence, but not be included
    * in the expansion.
    */
   for (size_t i = exp->start; i < exp->end; i++) {
      if (sent[i].len != sent[abbr].len)
         continue;
      if (!memcmp(sent[i].str, sent[abbr].str, sent[abbr].len))
         return false;
   }
   return true;
}

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

/* Forcibly truncate too long expansions. Mainly, to avoid overflowing the stack
 * during recursion.
 */
#define MAX_EXPANSION_LEN 100

static int find_acronym(struct gourgandine *rec, const struct mr_token *sent,
                        struct span *exp, struct span *abbr,
                        struct gn_acronym *acr)
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
   if (abbr->end - abbr->start != 1)
      goto reverse;

   /* Try the form <expansion> (<acronym>).
    *
    * Hearst requires that an expansion doesn't contain more tokens than:
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
   if (exp->end - exp->start > MAX_EXPANSION_LEN)
      exp->start = exp->end - MAX_EXPANSION_LEN;
   if (!pre_check(&sent[abbr->start]))
      goto reverse;
   if (!extract_rev(rec, sent, abbr->start, exp))
      goto reverse;
   if (!post_check(sent, abbr->start, exp))
      goto reverse;

   acr->acronym_start = abbr->start;
   acr->acronym_end = abbr->end;
   acr->expansion_start = exp->start;
   acr->expansion_end = exp->end;
   return 1;

reverse:
   /* Try the form <acronym> (<expansion>). We only look for an acronym
    * comprising a single token, but could also try with two token.
    */
   exp->start = exp->end - 1;
   if (abbr->end - abbr->start > MAX_EXPANSION_LEN)
      abbr->start = abbr->end - MAX_EXPANSION_LEN;
   if (!pre_check(&sent[exp->start]))
      return 0;
   if (!extract_fwd(rec, sent, exp->start, abbr))
      return 0;
   if (!post_check(sent, exp->start, abbr))
      return 0;

   acr->acronym_start = exp->start;
   acr->acronym_end = exp->end;
   acr->expansion_start = abbr->start;
   acr->expansion_end = abbr->end;
   return 1;
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

int gn_search(struct gourgandine *rec, const struct mr_token *sent, size_t len,
              struct gn_acronym *acr)
{
   struct span left, right;
   size_t lb, rb;
   size_t i;

   if (acr->acronym_start > acr->expansion_end) {
      /* <expansion> (<acronym>) <to_check...> */
      i = left.start = acr->acronym_end + 1;
   } else if (acr->expansion_end) {
      /* <acronym> (<expansion>)? <to_check...> */
      i = left.start = acr->expansion_end;
   } else {
       /* <to_check...>
        * Start i at 1 because there must be at least one token before the first
        * opening bracket.
        */
      left.start = 0;
      i = 1;
   }

   /* End at len - 1 because the opening bracket must be followed by at least
    * one token (and maybe a closing bracket).
    */
   for ( ; i + 1 < len; i++) {
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
         if (find_acronym(rec, sent, &left, &right, acr)) {
            gn_extract(rec, sent, acr);
            return 1;
         }
      }
   }
   return 0;
}

struct gourgandine *gn_alloc(void)
{
   struct gourgandine *gn = gn_malloc(sizeof *gn);
   *gn = (struct gourgandine){
      .buf = GN_VEC_INIT,
      .str = GN_VEC_INIT,
      .tokens = GN_VEC_INIT,
   };
   return gn;
}

void gn_dealloc(struct gourgandine *gn)
{
   gn_vec_free(gn->buf);
   gn_vec_free(gn->str);
   gn_vec_free(gn->tokens);
   free(gn);
}
