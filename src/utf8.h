#ifndef GN_UTF8_H
#define GN_UTF8_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "imp.h"

local bool gn_is_alnum(char32_t c);
local bool gn_is_double_quote(int32_t c);

#endif
