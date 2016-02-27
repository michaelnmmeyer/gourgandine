#include "lib/utf8proc.h"
#include "utf8.h"

local bool gn_is_alnum(char32_t c)
{
   const unsigned mask = (1 << KB_CATEGORY_LU)
                       | (1 << KB_CATEGORY_LL)
                       | (1 << KB_CATEGORY_LT)
                       | (1 << KB_CATEGORY_LM)
                       | (1 << KB_CATEGORY_LO)
                       | (1 << KB_CATEGORY_ND)
                       | (1 << KB_CATEGORY_NL)
                       | (1 << KB_CATEGORY_NO)
                       ;

   return (1 << kb_category(c)) & mask;
}

local bool gn_is_double_quote(int32_t c)
{
   switch (c) {
   case U'"': case U'”': case U'“': case U'„': case U'«': case U'»':
      return true;
   default:
      return false;
   }
}
