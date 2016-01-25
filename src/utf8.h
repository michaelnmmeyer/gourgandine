#ifndef GN_UTF8_H
#define GN_UTF8_H

#include <stddef.h>
#include <stdbool.h>

/* Character classes. */
enum {
   GN_LOWER = 1 << 0,    /* Lu */
   GN_UPPER = 1 << 1,    /* Ll */
   GN_ALPHA = 1 << 2,    /* Lu Ll Lt Lm Lo */
   GN_DIGIT = 1 << 3,    /* Nd Nl No */
   GN_WHITESPACE = 1 << 4,    /* Cc Zs Zl */
};

/* Returns a codepoint's class (bitwise of the relevant categories). */
unsigned gn_char_class(int32_t c);

/* Encodes a single code point, returns the number of bytes written. */
size_t mb_encode_char(char *dest, const int32_t c);

/* Decodes a single code point, returns the number of bytes read. */
size_t mb_decode_char(int32_t *restrict dest, const char *restrict str);

/* Decodes a string.
 * The destination buffer should be large enough to hold (len + 1) code points.
 * The output string is zero-terminated.
 * Returns the length of the decoded string.
 */
size_t mb_utf8_decode(int32_t *restrict dest,
                      const char *restrict str, size_t len);

/* Encodes a string.
 * The destination buffer should be large enough to hold (len * 4 + 1) bytes.
 * The output string buffer is zero-terminated. Returns the length of the
 * encoded string.
 */
size_t mb_utf8_encode(char *restrict dest,
                      const int32_t *restrict str, size_t len);

/* Returns the number of code points in a UTF-8-encoded string. */
size_t gn_utf8_len(const char *str, size_t len);

#endif
