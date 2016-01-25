#ifndef GN_UTF8_H
#define GN_UTF8_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool gn_is_upper(int32_t c);
bool gn_is_alnum(int32_t c);
bool gn_is_alpha(int32_t c);
bool gn_is_space(int32_t c);
bool gn_is_double_quote(int32_t c);

size_t gn_encode_char(char *dest, const int32_t c);
size_t gn_decode_char(int32_t *restrict dest, const char *restrict str);

size_t gn_utf8_len(const char *str, size_t len);

#endif
