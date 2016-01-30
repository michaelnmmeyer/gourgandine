#include "lib/utf8proc.h"
#include "utf8.h"

size_t gn_utf8_len(const char *str, size_t len)
{
   size_t ulen = 0;

   size_t clen;
   for (size_t i = 0; i < len; i += clen) {
      clen = utf8proc_utf8class[(uint8_t)str[i]];
      ulen++;
   }
   return ulen;
}

bool gn_is_upper(int32_t c)
{
   assert(utf8proc_codepoint_valid(c));

   switch (utf8proc_get_property(c)->category) {
   case UTF8PROC_CATEGORY_LU:
      return true;
   default:
      return false;
   }
}

bool gn_is_alnum(int32_t c)
{
   assert(utf8proc_codepoint_valid(c));

   switch (utf8proc_get_property(c)->category) {
   case UTF8PROC_CATEGORY_LU:
   case UTF8PROC_CATEGORY_LL:
   case UTF8PROC_CATEGORY_LT:
   case UTF8PROC_CATEGORY_LM:
   case UTF8PROC_CATEGORY_LO:
   case UTF8PROC_CATEGORY_ND:
   case UTF8PROC_CATEGORY_NL:
   case UTF8PROC_CATEGORY_NO:
      return true;
   default:
      return false;
   }
}

bool gn_is_alpha(int32_t c)
{
   assert(utf8proc_codepoint_valid(c));

   switch (utf8proc_get_property(c)->category) {
   case UTF8PROC_CATEGORY_LU:
   case UTF8PROC_CATEGORY_LL:
   case UTF8PROC_CATEGORY_LT:
   case UTF8PROC_CATEGORY_LM:
   case UTF8PROC_CATEGORY_LO:
      return true;
   default:
      return false;
   }
}

bool gn_is_space(int32_t c)
{
   switch (utf8proc_get_property(c)->category) {
   case UTF8PROC_CATEGORY_CC:
   case UTF8PROC_CATEGORY_ZS:
   case UTF8PROC_CATEGORY_ZL:
      return true;
   default:
      return false;
   }
}

bool gn_is_double_quote(int32_t c)
{
   switch (c) {
   case U'"': case U'”': case U'“': case U'„': case U'«': case U'»':
      return true;
   default:
      return false;
   }
}

size_t gn_decode_char(int32_t *restrict dest, const char *restrict str)
{
   const size_t len = utf8proc_utf8class[(uint8_t)*str];

   switch (len) {
   case 1:
      *dest = str[0];
      assert(utf8proc_codepoint_valid(*dest));
      return 1;
   case 2:
      *dest = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
      assert(utf8proc_codepoint_valid(*dest));
      return 2;
   case 3:
      *dest = ((str[0] & 0x0f) << 12) | ((str[1] & 0x3f) << 6)
            | (str[2] & 0x3f);
      assert(utf8proc_codepoint_valid(*dest));
      return 3;
   default:
      assert(len == 4);
      *dest = ((str[0] & 0x07) << 18) | ((str[1] & 0x3f) << 12)
            | ((str[2] & 0x3f) << 6) | (str[3] & 0x3f);
      assert(utf8proc_codepoint_valid(*dest));
      return 4;
   }
}

size_t gn_encode_char(char *dest, const int32_t c)
{
   assert(utf8proc_codepoint_valid(c));

   if (c < 0x80) {
      *dest = c;
      return 1;
   }
   if (c < 0x800) {
      dest[0] = 0xc0 | ((c & 0x07c0) >> 6);
      dest[1] = 0x80 | (c & 0x003f);
      return 2;
   }
   if (c < 0x10000) {
      dest[0] = 0xe0 | ((c & 0xf000) >> 12);
      dest[1] = 0x80 | ((c & 0x0fc0) >>  6);
      dest[2] = 0x80 | (c & 0x003f);
      return 3;
   }
   assert(c < 0x110000);
   dest[0] = 0xf0 | ((c & 0x1c0000) >> 18);
   dest[1] = 0x80 | ((c & 0x03f000) >> 12);
   dest[2] = 0x80 | ((c & 0x000fc0) >>  6);
   dest[3] = 0x80 | (c & 0x00003f);
   return 4;
}
