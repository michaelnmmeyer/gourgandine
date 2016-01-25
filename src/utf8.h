#ifndef GN_UTF8_H
#define GN_UTF8_H

#include <stddef.h>
#include <stdbool.h>

bool gn_is_upper(int32_t c);
bool gn_is_alnum(int32_t c);
bool gn_is_alpha(int32_t c);
bool gn_is_space(int32_t c);
bool gn_is_double_quote(int32_t c);

/* Encodes a single code point, returns the number of bytes written. */
size_t gn_encode_char(char *dest, const int32_t c);

/* Decodes a single code point, returns the number of bytes read. */
size_t gn_decode_char(int32_t *restrict dest, const char *restrict str);

/* Returns the number of code points in a UTF-8-encoded string. */
size_t gn_utf8_len(const char *str, size_t len);

#endif
