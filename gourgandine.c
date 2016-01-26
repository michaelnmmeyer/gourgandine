#line 1 "encode.c"
#include <assert.h>
#line 1 "utf8proc.h"
/*
 * Copyright (c) 2015 Steven G. Johnson, Jiahao Chen, Peter Colberg, Tony Kelman, Scott P. Jones, and other contributors.
 * Copyright (c) 2009 Public Software Group e. V., Berlin, Germany
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


/**
 * @mainpage
 *
 * utf8proc is a free/open-source (MIT/expat licensed) C library
 * providing Unicode normalization, case-folding, and other operations
 * for strings in the UTF-8 encoding, supporting Unicode version
 * 8.0.0.  See the utf8proc home page (http://julialang.org/utf8proc/)
 * for downloads and other information, or the source code on github
 * (https://github.com/JuliaLang/utf8proc).
 *
 * For the utf8proc API documentation, see: @ref utf8proc.h
 *
 * The features of utf8proc include:
 *
 * - Transformation of strings (@ref utf8proc_map) to:
 *    - decompose (@ref UTF8PROC_DECOMPOSE) or compose (@ref UTF8PROC_COMPOSE) Unicode combining characters (http://en.wikipedia.org/wiki/Combining_character)
 *    - canonicalize Unicode compatibility characters (@ref UTF8PROC_COMPAT)
 *    - strip "ignorable" (@ref UTF8PROC_IGNORE) characters, control characters (@ref UTF8PROC_STRIPCC), or combining characters such as accents (@ref UTF8PROC_STRIPMARK)
 *    - case-folding (@ref UTF8PROC_CASEFOLD)
 * - Unicode normalization: @ref utf8proc_NFD, @ref utf8proc_NFC, @ref utf8proc_NFKD, @ref utf8proc_NFKC
 * - Detecting grapheme boundaries (@ref utf8proc_grapheme_break and @ref UTF8PROC_CHARBOUND)
 * - Character-width computation: @ref utf8proc_charwidth
 * - Classification of characters by Unicode category: @ref utf8proc_category and @ref utf8proc_category_string
 * - Encode (@ref utf8proc_encode_char) and decode (@ref utf8proc_iterate) Unicode codepoints to/from UTF-8.
 */

/** @file */

#ifndef UTF8PROC_H
#define UTF8PROC_H

/** @name API version
 *
 * The utf8proc API version MAJOR.MINOR.PATCH, following
 * semantic-versioning rules (http://semver.org) based on API
 * compatibility.
 *
 * This is also returned at runtime by @ref utf8proc_version; however, the
 * runtime version may append a string like "-dev" to the version number
 * for prerelease versions.
 *
 * @note The shared-library version number in the Makefile may be different,
 *       being based on ABI compatibility rather than API compatibility.
 */
/** @{ */
/** The MAJOR version number (increased when backwards API compatibility is broken). */
#define UTF8PROC_VERSION_MAJOR 1
/** The MINOR version number (increased when new functionality is added in a backwards-compatible manner). */
#define UTF8PROC_VERSION_MINOR 3
/** The PATCH version (increased for fixes that do not change the API). */
#define UTF8PROC_VERSION_PATCH 0
/** @} */

#include <stdlib.h>
#include <sys/types.h>
#ifdef _MSC_VER
typedef signed char utf8proc_int8_t;
typedef unsigned char utf8proc_uint8_t;
typedef short utf8proc_int16_t;
typedef unsigned short utf8proc_uint16_t;
typedef int utf8proc_int32_t;
typedef unsigned int utf8proc_uint32_t;
#  ifdef _WIN64
typedef __int64 utf8proc_ssize_t;
typedef unsigned __int64 utf8proc_size_t;
#  else
typedef int utf8proc_ssize_t;
typedef unsigned int utf8proc_size_t;
#  endif
#  ifndef __cplusplus
typedef unsigned char utf8proc_bool;
enum {false, true};
#  else
typedef bool utf8proc_bool;
#  endif
#else
#  include <stdbool.h>
#  include <inttypes.h>
typedef int8_t utf8proc_int8_t;
typedef uint8_t utf8proc_uint8_t;
typedef int16_t utf8proc_int16_t;
typedef uint16_t utf8proc_uint16_t;
typedef int32_t utf8proc_int32_t;
typedef uint32_t utf8proc_uint32_t;
typedef size_t utf8proc_size_t;
typedef ssize_t utf8proc_ssize_t;
typedef bool utf8proc_bool;
#endif
#include <limits.h>

#ifdef _WIN32
#  ifdef UTF8PROC_EXPORTS
#    define UTF8PROC_DLLEXPORT __declspec(dllexport)
#  else
#    define UTF8PROC_DLLEXPORT __declspec(dllimport)
#  endif
#elif __GNUC__ >= 4
#  define UTF8PROC_DLLEXPORT __attribute__ ((visibility("default")))
#else
#  define UTF8PROC_DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX ((size_t)SIZE_MAX/2)
#endif

#ifndef UINT16_MAX
#  define UINT16_MAX ~(utf8proc_uint16_t)0
#endif

/**
 * Option flags used by several functions in the library.
 */
typedef enum {
  /** The given UTF-8 input is NULL terminated. */
  UTF8PROC_NULLTERM  = (1<<0),
  /** Unicode Versioning Stability has to be respected. */
  UTF8PROC_STABLE    = (1<<1),
  /** Compatibility decomposition (i.e. formatting information is lost). */
  UTF8PROC_COMPAT    = (1<<2),
  /** Return a result with decomposed characters. */
  UTF8PROC_COMPOSE   = (1<<3),
  /** Return a result with decomposed characters. */
  UTF8PROC_DECOMPOSE = (1<<4),
  /** Strip "default ignorable characters" such as SOFT-HYPHEN or ZERO-WIDTH-SPACE. */
  UTF8PROC_IGNORE    = (1<<5),
  /** Return an error, if the input contains unassigned codepoints. */
  UTF8PROC_REJECTNA  = (1<<6),
  /**
   * Indicating that NLF-sequences (LF, CRLF, CR, NEL) are representing a
   * line break, and should be converted to the codepoint for line
   * separation (LS).
   */
  UTF8PROC_NLF2LS    = (1<<7),
  /**
   * Indicating that NLF-sequences are representing a paragraph break, and
   * should be converted to the codepoint for paragraph separation
   * (PS).
   */
  UTF8PROC_NLF2PS    = (1<<8),
  /** Indicating that the meaning of NLF-sequences is unknown. */
  UTF8PROC_NLF2LF    = (UTF8PROC_NLF2LS | UTF8PROC_NLF2PS),
  /** Strips and/or convers control characters.
   *
   * NLF-sequences are transformed into space, except if one of the
   * NLF2LS/PS/LF options is given. HorizontalTab (HT) and FormFeed (FF)
   * are treated as a NLF-sequence in this case.  All other control
   * characters are simply removed.
   */
  UTF8PROC_STRIPCC   = (1<<9),
  /**
   * Performs unicode case folding, to be able to do a case-insensitive
   * string comparison.
   */
  UTF8PROC_CASEFOLD  = (1<<10),
  /**
   * Inserts 0xFF bytes at the beginning of each sequence which is
   * representing a single grapheme cluster (see UAX#29).
   */
  UTF8PROC_CHARBOUND = (1<<11),
  /** Lumps certain characters together.
   *
   * E.g. HYPHEN U+2010 and MINUS U+2212 to ASCII "-". See lump.md for details.
   *
   * If NLF2LF is set, this includes a transformation of paragraph and
   * line separators to ASCII line-feed (LF).
   */
  UTF8PROC_LUMP      = (1<<12),
  /** Strips all character markings.
   *
   * This includes non-spacing, spacing and enclosing (i.e. accents).
   * @note This option works only with @ref UTF8PROC_COMPOSE or
   *       @ref UTF8PROC_DECOMPOSE
   */
  UTF8PROC_STRIPMARK = (1<<13),
} utf8proc_option_t;

/** @name Error codes
 * Error codes being returned by almost all functions.
 */
/** @{ */
/** Memory could not be allocated. */
#define UTF8PROC_ERROR_NOMEM -1
/** The given string is too long to be processed. */
#define UTF8PROC_ERROR_OVERFLOW -2
/** The given string is not a legal UTF-8 string. */
#define UTF8PROC_ERROR_INVALIDUTF8 -3
/** The @ref UTF8PROC_REJECTNA flag was set and an unassigned codepoint was found. */
#define UTF8PROC_ERROR_NOTASSIGNED -4
/** Invalid options have been used. */
#define UTF8PROC_ERROR_INVALIDOPTS -5
/** @} */

/* @name Types */

/** Holds the value of a property. */
typedef utf8proc_int16_t utf8proc_propval_t;

/** Struct containing information about a codepoint. */
typedef struct utf8proc_property_struct {
  /**
   * Unicode category.
   * @see utf8proc_category_t.
   */
  utf8proc_propval_t category;
  utf8proc_propval_t combining_class;
  /**
   * Bidirectional class.
   * @see utf8proc_bidi_class_t.
   */
  utf8proc_propval_t bidi_class;
  /**
   * @anchor Decomposition type.
   * @see utf8proc_decomp_type_t.
   */
  utf8proc_propval_t decomp_type;
  utf8proc_uint16_t decomp_mapping;
  utf8proc_uint16_t casefold_mapping;
  utf8proc_int32_t uppercase_mapping;
  utf8proc_int32_t lowercase_mapping;
  utf8proc_int32_t titlecase_mapping;
  utf8proc_int32_t comb1st_index;
  utf8proc_int32_t comb2nd_index;
  unsigned bidi_mirrored:1;
  unsigned comp_exclusion:1;
  /**
   * Can this codepoint be ignored?
   *
   * Used by @ref utf8proc_decompose_char when @ref UTF8PROC_IGNORE is
   * passed as an option.
   */
  unsigned ignorable:1;
  unsigned control_boundary:1;
  /**
   * Boundclass.
   * @see utf8proc_boundclass_t.
   */
  unsigned boundclass:4;
  /** The width of the codepoint. */
  unsigned charwidth:2;
} utf8proc_property_t;

/** Unicode categories. */
typedef enum {
  UTF8PROC_CATEGORY_CN  = 0, /**< Other, not assigned */
  UTF8PROC_CATEGORY_LU  = 1, /**< Letter, uppercase */
  UTF8PROC_CATEGORY_LL  = 2, /**< Letter, lowercase */
  UTF8PROC_CATEGORY_LT  = 3, /**< Letter, titlecase */
  UTF8PROC_CATEGORY_LM  = 4, /**< Letter, modifier */
  UTF8PROC_CATEGORY_LO  = 5, /**< Letter, other */
  UTF8PROC_CATEGORY_MN  = 6, /**< Mark, nonspacing */
  UTF8PROC_CATEGORY_MC  = 7, /**< Mark, spacing combining */
  UTF8PROC_CATEGORY_ME  = 8, /**< Mark, enclosing */
  UTF8PROC_CATEGORY_ND  = 9, /**< Number, decimal digit */
  UTF8PROC_CATEGORY_NL = 10, /**< Number, letter */
  UTF8PROC_CATEGORY_NO = 11, /**< Number, other */
  UTF8PROC_CATEGORY_PC = 12, /**< Punctuation, connector */
  UTF8PROC_CATEGORY_PD = 13, /**< Punctuation, dash */
  UTF8PROC_CATEGORY_PS = 14, /**< Punctuation, open */
  UTF8PROC_CATEGORY_PE = 15, /**< Punctuation, close */
  UTF8PROC_CATEGORY_PI = 16, /**< Punctuation, initial quote */
  UTF8PROC_CATEGORY_PF = 17, /**< Punctuation, final quote */
  UTF8PROC_CATEGORY_PO = 18, /**< Punctuation, other */
  UTF8PROC_CATEGORY_SM = 19, /**< Symbol, math */
  UTF8PROC_CATEGORY_SC = 20, /**< Symbol, currency */
  UTF8PROC_CATEGORY_SK = 21, /**< Symbol, modifier */
  UTF8PROC_CATEGORY_SO = 22, /**< Symbol, other */
  UTF8PROC_CATEGORY_ZS = 23, /**< Separator, space */
  UTF8PROC_CATEGORY_ZL = 24, /**< Separator, line */
  UTF8PROC_CATEGORY_ZP = 25, /**< Separator, paragraph */
  UTF8PROC_CATEGORY_CC = 26, /**< Other, control */
  UTF8PROC_CATEGORY_CF = 27, /**< Other, format */
  UTF8PROC_CATEGORY_CS = 28, /**< Other, surrogate */
  UTF8PROC_CATEGORY_CO = 29, /**< Other, private use */
} utf8proc_category_t;

/** Bidirectional character classes. */
typedef enum {
  UTF8PROC_BIDI_CLASS_L     = 1, /**< Left-to-Right */
  UTF8PROC_BIDI_CLASS_LRE   = 2, /**< Left-to-Right Embedding */
  UTF8PROC_BIDI_CLASS_LRO   = 3, /**< Left-to-Right Override */
  UTF8PROC_BIDI_CLASS_R     = 4, /**< Right-to-Left */
  UTF8PROC_BIDI_CLASS_AL    = 5, /**< Right-to-Left Arabic */
  UTF8PROC_BIDI_CLASS_RLE   = 6, /**< Right-to-Left Embedding */
  UTF8PROC_BIDI_CLASS_RLO   = 7, /**< Right-to-Left Override */
  UTF8PROC_BIDI_CLASS_PDF   = 8, /**< Pop Directional Format */
  UTF8PROC_BIDI_CLASS_EN    = 9, /**< European Number */
  UTF8PROC_BIDI_CLASS_ES   = 10, /**< European Separator */
  UTF8PROC_BIDI_CLASS_ET   = 11, /**< European Number Terminator */
  UTF8PROC_BIDI_CLASS_AN   = 12, /**< Arabic Number */
  UTF8PROC_BIDI_CLASS_CS   = 13, /**< Common Number Separator */
  UTF8PROC_BIDI_CLASS_NSM  = 14, /**< Nonspacing Mark */
  UTF8PROC_BIDI_CLASS_BN   = 15, /**< Boundary Neutral */
  UTF8PROC_BIDI_CLASS_B    = 16, /**< Paragraph Separator */
  UTF8PROC_BIDI_CLASS_S    = 17, /**< Segment Separator */
  UTF8PROC_BIDI_CLASS_WS   = 18, /**< Whitespace */
  UTF8PROC_BIDI_CLASS_ON   = 19, /**< Other Neutrals */
  UTF8PROC_BIDI_CLASS_LRI  = 20, /**< Left-to-Right Isolate */
  UTF8PROC_BIDI_CLASS_RLI  = 21, /**< Right-to-Left Isolate */
  UTF8PROC_BIDI_CLASS_FSI  = 22, /**< First Strong Isolate */
  UTF8PROC_BIDI_CLASS_PDI  = 23, /**< Pop Directional Isolate */
} utf8proc_bidi_class_t;

/** Decomposition type. */
typedef enum {
  UTF8PROC_DECOMP_TYPE_FONT      = 1, /**< Font */
  UTF8PROC_DECOMP_TYPE_NOBREAK   = 2, /**< Nobreak */
  UTF8PROC_DECOMP_TYPE_INITIAL   = 3, /**< Initial */
  UTF8PROC_DECOMP_TYPE_MEDIAL    = 4, /**< Medial */
  UTF8PROC_DECOMP_TYPE_FINAL     = 5, /**< Final */
  UTF8PROC_DECOMP_TYPE_ISOLATED  = 6, /**< Isolated */
  UTF8PROC_DECOMP_TYPE_CIRCLE    = 7, /**< Circle */
  UTF8PROC_DECOMP_TYPE_SUPER     = 8, /**< Super */
  UTF8PROC_DECOMP_TYPE_SUB       = 9, /**< Sub */
  UTF8PROC_DECOMP_TYPE_VERTICAL = 10, /**< Vertical */
  UTF8PROC_DECOMP_TYPE_WIDE     = 11, /**< Wide */
  UTF8PROC_DECOMP_TYPE_NARROW   = 12, /**< Narrow */
  UTF8PROC_DECOMP_TYPE_SMALL    = 13, /**< Small */
  UTF8PROC_DECOMP_TYPE_SQUARE   = 14, /**< Square */
  UTF8PROC_DECOMP_TYPE_FRACTION = 15, /**< Fraction */
  UTF8PROC_DECOMP_TYPE_COMPAT   = 16, /**< Compat */
} utf8proc_decomp_type_t;

/** Boundclass property. */
typedef enum {
  UTF8PROC_BOUNDCLASS_START              =  0, /**< Start */
  UTF8PROC_BOUNDCLASS_OTHER              =  1, /**< Other */
  UTF8PROC_BOUNDCLASS_CR                 =  2, /**< Cr */
  UTF8PROC_BOUNDCLASS_LF                 =  3, /**< Lf */
  UTF8PROC_BOUNDCLASS_CONTROL            =  4, /**< Control */
  UTF8PROC_BOUNDCLASS_EXTEND             =  5, /**< Extend */
  UTF8PROC_BOUNDCLASS_L                  =  6, /**< L */
  UTF8PROC_BOUNDCLASS_V                  =  7, /**< V */
  UTF8PROC_BOUNDCLASS_T                  =  8, /**< T */
  UTF8PROC_BOUNDCLASS_LV                 =  9, /**< Lv */
  UTF8PROC_BOUNDCLASS_LVT                = 10, /**< Lvt */
  UTF8PROC_BOUNDCLASS_REGIONAL_INDICATOR = 11, /**< Regional indicator */
  UTF8PROC_BOUNDCLASS_SPACINGMARK        = 12, /**< Spacingmark */
} utf8proc_boundclass_t;

/**
 * Array containing the byte lengths of a UTF-8 encoded codepoint based
 * on the first byte.
 */
UTF8PROC_DLLEXPORT extern const utf8proc_int8_t utf8proc_utf8class[256];

/**
 * Returns the utf8proc API version as a string MAJOR.MINOR.PATCH
 * (http://semver.org format), possibly with a "-dev" suffix for
 * development versions.
 */
UTF8PROC_DLLEXPORT const char *utf8proc_version(void);

/**
 * Returns an informative error string for the given utf8proc error code
 * (e.g. the error codes returned by @ref utf8proc_map).
 */
UTF8PROC_DLLEXPORT const char *utf8proc_errmsg(utf8proc_ssize_t errcode);

/**
 * Reads a single codepoint from the UTF-8 sequence being pointed to by `str`.
 * The maximum number of bytes read is `strlen`, unless `strlen` is
 * negative (in which case up to 4 bytes are read).
 *
 * If a valid codepoint could be read, it is stored in the variable
 * pointed to by `codepoint_ref`, otherwise that variable will be set to -1.
 * In case of success, the number of bytes read is returned; otherwise, a
 * negative error code is returned.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_iterate(const utf8proc_uint8_t *str, utf8proc_ssize_t strlen, utf8proc_int32_t *codepoint_ref);

/**
 * Check if a codepoint is valid (regardless of whether it has been
 * assigned a value by the current Unicode standard).
 *
 * @return 1 if the given `codepoint` is valid and otherwise return 0.
 */
UTF8PROC_DLLEXPORT utf8proc_bool utf8proc_codepoint_valid(utf8proc_int32_t codepoint);

/**
 * Encodes the codepoint as an UTF-8 string in the byte array pointed
 * to by `dst`. This array must be at least 4 bytes long.
 *
 * In case of success the number of bytes written is returned, and
 * otherwise 0 is returned.
 *
 * This function does not check whether `codepoint` is valid Unicode.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_encode_char(utf8proc_int32_t codepoint, utf8proc_uint8_t *dst);

/**
 * Look up the properties for a given codepoint.
 *
 * @param codepoint The Unicode codepoint.
 *
 * @returns
 * A pointer to a (constant) struct containing information about
 * the codepoint.
 * @par
 * If the codepoint is unassigned or invalid, a pointer to a special struct is
 * returned in which `category` is 0 (@ref UTF8PROC_CATEGORY_CN).
 */
UTF8PROC_DLLEXPORT const utf8proc_property_t *utf8proc_get_property(utf8proc_int32_t codepoint);

/** Decompose a codepoint into an array of codepoints.
 *
 * @param codepoint the codepoint.
 * @param dst the destination buffer.
 * @param bufsize the size of the destination buffer.
 * @param options one or more of the following flags:
 * - @ref UTF8PROC_REJECTNA  - return an error `codepoint` is unassigned
 * - @ref UTF8PROC_IGNORE    - strip "default ignorable" codepoints
 * - @ref UTF8PROC_CASEFOLD  - apply Unicode casefolding
 * - @ref UTF8PROC_COMPAT    - replace certain codepoints with their
 *                             compatibility decomposition
 * - @ref UTF8PROC_CHARBOUND - insert 0xFF bytes before each grapheme cluster
 * - @ref UTF8PROC_LUMP      - lump certain different codepoints together
 * - @ref UTF8PROC_STRIPMARK - remove all character marks
 * @param last_boundclass
 * Pointer to an integer variable containing
 * the previous codepoint's boundary class if the @ref UTF8PROC_CHARBOUND
 * option is used.  Otherwise, this parameter is ignored.
 *
 * @return
 * In case of success, the number of codepoints written is returned; in case
 * of an error, a negative error code is returned (@ref utf8proc_errmsg).
 * @par
 * If the number of written codepoints would be bigger than `bufsize`, the
 * required buffer size is returned, while the buffer will be overwritten with
 * undefined data.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_decompose_char(
  utf8proc_int32_t codepoint, utf8proc_int32_t *dst, utf8proc_ssize_t bufsize,
  utf8proc_option_t options, int *last_boundclass
);

/**
 * The same as @ref utf8proc_decompose_char, but acts on a whole UTF-8
 * string and orders the decomposed sequences correctly.
 *
 * If the @ref UTF8PROC_NULLTERM flag in `options` is set, processing
 * will be stopped, when a NULL byte is encounted, otherwise `strlen`
 * bytes are processed.  The result (in the form of 32-bit unicode
 * codepoints) is written into the buffer being pointed to by
 * `buffer` (which must contain at least `bufsize` entries).  In case of
 * success, the number of codepoints written is returned; in case of an
 * error, a negative error code is returned (@ref utf8proc_errmsg).
 *
 * If the number of written codepoints would be bigger than `bufsize`, the
 * required buffer size is returned, while the buffer will be overwritten with
 * undefined data.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_decompose(
  const utf8proc_uint8_t *str, utf8proc_ssize_t strlen,
  utf8proc_int32_t *buffer, utf8proc_ssize_t bufsize, utf8proc_option_t options
);

/**
 * Reencodes the sequence of `length` codepoints pointed to by `buffer`
 * UTF-8 data in-place (i.e., the result is also stored in `buffer`).
 *
 * @param buffer the (native-endian UTF-32) unicode codepoints to re-encode.
 * @param length the length (in codepoints) of the buffer.
 * @param options a bitwise or (`|`) of one or more of the following flags:
 * - @ref UTF8PROC_NLF2LS  - convert LF, CRLF, CR and NEL into LS
 * - @ref UTF8PROC_NLF2PS  - convert LF, CRLF, CR and NEL into PS
 * - @ref UTF8PROC_NLF2LF  - convert LF, CRLF, CR and NEL into LF
 * - @ref UTF8PROC_STRIPCC - strip or convert all non-affected control characters
 * - @ref UTF8PROC_COMPOSE - try to combine decomposed codepoints into composite
 *                           codepoints
 * - @ref UTF8PROC_STABLE  - prohibit combining characters that would violate
 *                           the unicode versioning stability
 *
 * @return
 * In case of success, the length (in bytes) of the resulting UTF-8 string is
 * returned; otherwise, a negative error code is returned (@ref utf8proc_errmsg).
 *
 * @warning The amount of free space pointed to by `buffer` must
 *          exceed the amount of the input data by one byte, and the
 *          entries of the array pointed to by `str` have to be in the
 *          range `0x0000` to `0x10FFFF`. Otherwise, the program might crash!
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_reencode(utf8proc_int32_t *buffer, utf8proc_ssize_t length, utf8proc_option_t options);

/**
 * Given a pair of consecutive codepoints, return whether a grapheme break is
 * permitted between them (as defined by the extended grapheme clusters in UAX#29).
 */
UTF8PROC_DLLEXPORT utf8proc_bool utf8proc_grapheme_break(utf8proc_int32_t codepoint1, utf8proc_int32_t codepoint2);


/**
 * Given a codepoint `c`, return the codepoint of the corresponding
 * lower-case character, if any; otherwise (if there is no lower-case
 * variant, or if `c` is not a valid codepoint) return `c`.
 */
UTF8PROC_DLLEXPORT utf8proc_int32_t utf8proc_tolower(utf8proc_int32_t c);

/**
 * Given a codepoint `c`, return the codepoint of the corresponding
 * upper-case character, if any; otherwise (if there is no upper-case
 * variant, or if `c` is not a valid codepoint) return `c`.
 */
UTF8PROC_DLLEXPORT utf8proc_int32_t utf8proc_toupper(utf8proc_int32_t c);

/**
 * Given a codepoint, return a character width analogous to `wcwidth(codepoint)`,
 * except that a width of 0 is returned for non-printable codepoints
 * instead of -1 as in `wcwidth`.
 *
 * @note
 * If you want to check for particular types of non-printable characters,
 * (analogous to `isprint` or `iscntrl`), use @ref utf8proc_category. */
UTF8PROC_DLLEXPORT int utf8proc_charwidth(utf8proc_int32_t codepoint);

/**
 * Return the Unicode category for the codepoint (one of the
 * @ref utf8proc_category_t constants.)
 */
UTF8PROC_DLLEXPORT utf8proc_category_t utf8proc_category(utf8proc_int32_t codepoint);

/**
 * Return the two-letter (nul-terminated) Unicode category string for
 * the codepoint (e.g. `"Lu"` or `"Co"`).
 */
UTF8PROC_DLLEXPORT const char *utf8proc_category_string(utf8proc_int32_t codepoint);

/**
 * Maps the given UTF-8 string pointed to by `str` to a new UTF-8
 * string, allocated dynamically by `malloc` and returned via `dstptr`.
 *
 * If the @ref UTF8PROC_NULLTERM flag in the `options` field is set,
 * the length is determined by a NULL terminator, otherwise the
 * parameter `strlen` is evaluated to determine the string length, but
 * in any case the result will be NULL terminated (though it might
 * contain NULL characters with the string if `str` contained NULL
 * characters). Other flags in the `options` field are passed to the
 * functions defined above, and regarded as described.
 *
 * In case of success the length of the new string is returned,
 * otherwise a negative error code is returned.
 *
 * @note The memory of the new UTF-8 string will have been allocated
 * with `malloc`, and should therefore be deallocated with `free`.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_map(
  const utf8proc_uint8_t *str, utf8proc_ssize_t strlen, utf8proc_uint8_t **dstptr, utf8proc_option_t options
);

/** @name Unicode normalization
 *
 * Returns a pointer to newly allocated memory of a NFD, NFC, NFKD or NFKC
 * normalized version of the null-terminated string `str`.  These
 * are shortcuts to calling @ref utf8proc_map with @ref UTF8PROC_NULLTERM
 * combined with @ref UTF8PROC_STABLE and flags indicating the normalization.
 */
/** @{ */
/** NFD normalization (@ref UTF8PROC_DECOMPOSE). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFD(const utf8proc_uint8_t *str);
/** NFC normalization (@ref UTF8PROC_COMPOSE). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFC(const utf8proc_uint8_t *str);
/** NFD normalization (@ref UTF8PROC_DECOMPOSE and @ref UTF8PROC_COMPAT). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFKD(const utf8proc_uint8_t *str);
/** NFD normalization (@ref UTF8PROC_COMPOSE and @ref UTF8PROC_COMPAT). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFKC(const utf8proc_uint8_t *str);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
#line 3 "encode.c"
#line 1 "mascara.h"
#ifndef MASCARA_H
#define MASCARA_H

#define MR_VERSION "0.5"

#include <stddef.h>

/* Maximum allowed length of a sentence, in tokens. Sentences that would grow
 * larger than that are split in chunks. This is done to avoid pathological
 * cases.
 */
#define MR_MAX_SENTENCE_LEN 1000

/* See the readme file for informations about these. */
enum mr_token_type {
   MR_UNK,
   MR_LATIN,
   MR_PREFIX,
   MR_SUFFIX,
   MR_SYM,
   MR_NUM,
   MR_ABBR,
   MR_EMAIL,
   MR_URI,
   MR_PATH,
};

/* String representation of a token type. */
const char *mr_token_type_name(enum mr_token_type);

struct mascara;

/* Tokenization mode. */
enum mr_mode {
   MR_TOKEN,      /* Iterate over tokens. */
   MR_SENTENCE,   /* Iterate over sentences, where an sentence is an array of
                   * tokens. */
};

/* Returns an array containing the names of the supported languages.
 * The array is NULL-terminated.
 */
const char *const *mr_langs(void);

/* Allocates a new tokenizer.
 * If there is no implementation for the provided language name, returns NULL.
 * Available languages are "en", "fr", and "it".
 */
struct mascara *mr_alloc(const char *lang, enum mr_mode);

/* Destructor. */
void mr_dealloc(struct mascara *);

/* Returns the chosen tokenization mode. */
enum mr_mode mr_mode(const struct mascara *);

/* Sets the text to tokenize.
 * The input string must be valid UTF-8 and normalized to NFC. No internal check
 * is made to ensure that this is the case. If this isn't, the result is
 * undefined. The input string is not copied internally, and should then not be
 * deallocated until this function is called with a new string.
 */
void mr_set_text(struct mascara *, const char *str, size_t len);

struct mr_token {
   const char *str;           /* Not nul-terminated! */
   size_t len;                /* Length, in bytes. */
   size_t offset;             /* Offset from the start of the text, in bytes. */
   enum mr_token_type type;
};

/* Fetch the next token or sentence.
 * Must be called after mr_set_text().
 * The behaviour of this function depends on the chosen tokenization mode:
 * - If it is MR_TOKEN, looks for the next token in the input text. If there is
 *   one, makes the provided token pointer point to a structure filled with
 *   informations about it, and returns 1.
 * - If it is MR_SENTENCE, looks for the next sentence in the input text. If
 *   there is one, makes the provided token pointer point to an array of token
 *   structures, and returns the number of tokens in the sentence.
 * If at the end of the text, makes the provided token pointer point to NULL,
 * and returns 0.
 */
size_t mr_next(struct mascara *, struct mr_token **);

#endif
#line 4 "encode.c"
#line 1 "imp.h"
#ifndef GN_IMP_H
#define GN_IMP_H

#include <stdint.h>

struct span {
   size_t start;
   size_t end;
};

struct gourgandine {

   /* Buffer for normalizing an acronym and its expansion. They are stored
    * consecutively: acronym '\0' expansion '\0'.
    */
   char *buf;

   /* Buffer for holding the string to match, which is normalized. We write here
    * a string of the form: acronym TAB (expansion_word SPACE)+.
    */
   int32_t *str;
   
   /* Over-segmenting tokens is necessary for matching, e.g.:
    *
    *    [GAP] D-glyercaldehyde 3-phosphate
    *
    * Our tokenizer doesn't split on '-', in particular, so we must perform a
    * new segmentation of each token. The following keeps track of the relation
    * between the token chunks we produce here and the position of the
    * corresponding token in the input sentence, so that we can obtain correct
    * offsets after processing.
    */
   struct assoc {
      /* Offset in the "str" of the current normalized token. */
      size_t norm_off;
      /* Position of the corresponding real token in the sentence. */
      size_t token_no;
   } *tokens;
};

struct gn_acronym;

void gn_encode(struct gourgandine *rec, const struct mr_token *sent,
               size_t abbr, const struct span *exp);

void gn_extract(struct gourgandine *rec, const struct mr_token *sent,
                struct gn_acronym *def);
#endif
#line 5 "encode.c"
#line 1 "utf8.h"
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
#line 6 "encode.c"
#line 1 "vec.h"
#ifndef GN_VEC_H
#define GN_VEC_H

#include <stdlib.h>

extern size_t gn_vec_void[2];

#define GN_VEC_INIT (void *)&gn_vec_void[2]

#define gn_vec_header(vec) ((size_t *)((char *)(vec) - sizeof gn_vec_void))

#define gn_vec_len(vec)  gn_vec_header(vec)[0]
#define gn_vec_free(vec) gn_vec_free(gn_vec_header(vec))

#define gn_vec_grow(vec, nr) do {                                              \
   vec = gn_vec_grow(gn_vec_header(vec), nr, sizeof *(vec));                   \
} while (0)

#define gn_vec_clear(vec) do {                                                 \
   gn_vec_len(vec) = 0;                                                        \
} while (0)

#define gn_vec_push(vec, x) do {                                               \
   gn_vec_grow(vec, 1);                                                        \
   vec[gn_vec_len(vec)++] = x;                                                 \
} while (0)

void *(gn_vec_grow)(size_t *vec, size_t incr, size_t elt_size);

static inline void (gn_vec_free)(size_t *vec)
{
   if (vec != gn_vec_void)
      free(vec);
}

#endif
#line 7 "encode.c"

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
static int32_t *push_letter(int32_t *str, int32_t c)
{
   assert(gn_is_alpha(c));
   
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
      gn_vec_push(str, 'o');
      break;
   case U'Æ': case U'æ':
      /* Never actually encountered this ligature. I think it is reasonable to
       * assume that it is used like Œ.
       */
      gn_vec_push(str, 'a');
      break;
   default: {
      /* The longest latin glyphs, after NFKC normalization, are the ligatures ﬃ
       * and ﬄ. Some glyphs of other scripts produce much wider sequences, but
       * are useless for our purpose, so we just ignore them. Note that we don't
       * necessarily obtain a NFKC-normalized string: further processing is
       * required to do things properly (see utf8proc_decompose()), but we only
       * care about alphabetic characters encoded in a single code point, so
       * this doesn't matter.
       */
      int32_t cs[3];
      const size_t max = sizeof cs / sizeof *cs;
      ssize_t len = utf8proc_decompose_char(c, cs, max, norm_opts, NULL);
      if (len > 0 && (size_t)len <= max)
         for (ssize_t i = 0; i < len; i++)
            gn_vec_push(str, cs[i]);
   }
   }
   return str;
}

static void encode_abbr(struct gourgandine *rec, const struct mr_token *acr)
{
   assert(gn_vec_len(rec->str) == 0);

   for (size_t i = 0; i < acr->len; ) {
      int32_t c;
      i += gn_decode_char(&c, &acr->str[i]);
      if (gn_is_alpha(c))
         rec->str = push_letter(rec->str, c);
   }
}

static void encode_exp(struct gourgandine *rec, const struct span *exp,
                       const struct mr_token *sent)
{
   for (size_t t = exp->start; t < exp->end; t++) {
      bool in_token = false;
      const struct mr_token *token = &sent[t];
      for (size_t i = 0; i < token->len; ) {
         int32_t c;
         i += gn_decode_char(&c, &token->str[i]);
         if (gn_is_alpha(c)) {
            if (!in_token) {
               in_token = true;
               struct assoc a = {
                  .norm_off = gn_vec_len(rec->str),
                  .token_no = t,
               };
               gn_vec_push(rec->tokens, a);
            }
            rec->str = push_letter(rec->str, c);
         } else if (in_token) {
            gn_vec_push(rec->str, ' ');
            in_token = false;
         }
      }
      if (in_token)
         gn_vec_push(rec->str, ' ');
   }
}

void gn_encode(struct gourgandine *rec, const struct mr_token *sent,
               size_t abbr, const struct span *exp)
{
   gn_vec_clear(rec->str);
   gn_vec_clear(rec->tokens);
   
   encode_abbr(rec, &sent[abbr]);
   gn_vec_push(rec->str, '\t');
   encode_exp(rec, exp, sent);
   gn_vec_grow(rec->str, 1);
   rec->str[gn_vec_len(rec->str)] = '\0';
}
#line 1 "mem.c"
#include <stdlib.h>
#include <stdio.h>
#line 1 "mem.h"
#ifndef GN_MEM_H
#define GN_MEM_H

#include <stddef.h>
#include <stdarg.h>
#include <stdnoreturn.h>

noreturn void gn_fatal(const char *msg, ...);

void *gn_malloc(size_t)
#ifdef ___GNUC__
   __attribute__((malloc))
#endif
   ;

void *gn_realloc(void *, size_t);

#endif
#line 4 "mem.c"

noreturn void gn_fatal(const char *msg, ...)
{
   va_list ap;

   fputs("gourgandine: ", stderr);
   va_start(ap, msg);
   vfprintf(stderr, msg, ap);
   va_end(ap);
   putc('\n', stderr);
   abort();
}

#define GN_OOM() gn_fatal("out of memory")

void *gn_malloc(size_t size)
{
   assert(size);
   void *mem = malloc(size);
   if (!mem)
      GN_OOM();
   return mem;
}

void *gn_realloc(void *mem, size_t size)
{
   assert(size);
   mem = realloc(mem, size);
   if (!mem)
      GN_OOM();
   return mem;
}
#line 1 "normalize.c"
#line 1 "api.h"
#ifndef GOURGANDINE_H
#define GOURGANDINE_H

#define GN_VERSION "0.2"

#include <stddef.h>

struct gourgandine *gn_alloc(void);
void gn_dealloc(struct gourgandine *);

/* An acronym definition. */
struct gn_acronym {

   /* The acronym and its expansion, normalized: periods are removed in the
    * acronym, while double quotes and excessive white space are trimmed in the
    * corresponding expansion. These strings do not point into the source
    * text. They are nul-terminated.
    */
   const char *acronym;
   size_t acronym_len;
   const char *expansion;
   size_t expansion_len;

   /* Start and end offset, in the input sentence, of the acronym and its
    * expansion. We count in tokens. The end offset of the expansion is the
    * offset of the token that follows its last word in the input sentence,
    * such that substracting the expansion start offset from it gives the
    * length, in tokens, of the expansion. The same applies to the abbreviation.
    */
   size_t acronym_start;
   size_t acronym_end;
   size_t expansion_start;
   size_t expansion_end;
};

/* Declared in mascara.h (https://github.com/michaelnmmeyer/mascara). */
struct mr_token;

/* Finds acronym definitions in a sentence.
 *
 * If an acronym definition is found in the provided sentence, fills the
 * provided acronym structure with informations about it, and returns 1.
 * Otherwise, leaves the acronym structure untouched, and returns 0.
 *
 * This must be called several times in a loop to obtain all acronyms in a
 * sentence. Before the first call, the acronym structure must be zeroed.
 * Afterwards, the same structure must be passed again, untouched: the offsets
 * it contains are used to determine where to restart on each call.
 *
 * The provided sentence must be valid UTF-8. Otherwise, the result is
 * undefined.
 */
int gn_search(struct gourgandine *, const struct mr_token *sent, size_t sent_len,
              struct gn_acronym *);

#endif
#line 3 "normalize.c"

static size_t norm_exp(char *buf, const char *str, size_t len)
{
   size_t new_len = 0;

   for (size_t i = 0; i < len; ) {
      int32_t c;
      i += gn_decode_char(&c, &str[i]);
      
      /* Skip quotation marks. */
      if (gn_is_double_quote(c))
         continue;
      /* Reduce whitespace spans to a single space character. */
      if (gn_is_space(c)) {
         do {
            /* Trailing spaces should already be removed. */
            assert(i < len);
            i += gn_decode_char(&c, &str[i]);
         } while (gn_is_space(c));
         buf[new_len++] = ' ';
      }
      new_len += gn_encode_char(&buf[new_len], c);
   }
   return new_len;
}

static size_t norm_abbr(char *buf, const char *str, size_t len)
{
   size_t new_len = 0;

   /* Drop internal periods. */
   for (size_t i = 0; i < len; i++)
      if (str[i] != '.')
         buf[new_len++] = str[i];
   return new_len;
}

void gn_extract(struct gourgandine *rec, const struct mr_token *sent,
                struct gn_acronym *def)
{
   size_t acr_len = sent[def->acronym_start].len;
   size_t exp_len = sent[def->expansion_end - 1].offset
                  + sent[def->expansion_end - 1].len
                  - sent[def->expansion_start].offset;

   gn_vec_clear(rec->buf);
   gn_vec_grow(rec->buf, exp_len + 1 + acr_len + 1);

   exp_len = norm_exp(rec->buf, sent[def->expansion_start].str, exp_len);
   def->expansion = rec->buf;
   def->expansion_len = exp_len;
   rec->buf[exp_len] = '\0';

   acr_len = norm_abbr(&rec->buf[exp_len + 1], sent[def->acronym_start].str, acr_len);
   def->acronym = &rec->buf[exp_len + 1];
   def->acronym_len = acr_len;
   rec->buf[exp_len + 1 + acr_len] = '\0';
}
#line 1 "search.c"
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
   size_t ulen = gn_utf8_len(acr->str, acr->len);
   if (ulen < 2 || ulen > 10)
      return false;
   
   /* Require that the acronym's first character is alphanumeric.
    * Everybody does that, too.
    */
   int32_t c;
   gn_decode_char(&c, acr->str);
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
   size_t i = 0;
   while (i < acr->len) {
      i += gn_decode_char(&c, &acr->str[i]);
      if (gn_is_upper(c)) {
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
#line 1 "utf8.c"

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
#line 1 "vec.c"
#include <stdlib.h>

size_t gn_vec_void[2];

void *(gn_vec_grow)(size_t *vec, size_t nr, size_t elt_size)
{
   size_t need = vec[0] + nr;
   if (need < nr)
      gn_fatal("integer overflow");
   
   size_t alloc = vec[1];
   if (alloc >= need)
      return vec + 2;

   if (vec == gn_vec_void) {
      if (need > (SIZE_MAX - sizeof gn_vec_void) / elt_size)
         gn_fatal("integer overflow");
      vec = gn_malloc(sizeof gn_vec_void + need * elt_size);
      vec[0] = vec[1] = 0;
      return vec + 2;
   }
   
   alloc += (alloc >> 1);
   if (alloc < need)
      alloc = need;
   if (alloc > (SIZE_MAX - sizeof gn_vec_void) / elt_size)
      gn_fatal("integer overflow");
   vec = gn_realloc(vec, sizeof gn_vec_void + alloc * elt_size);
   vec[1] = alloc;
   return vec + 2;
}
