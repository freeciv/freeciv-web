/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__FC_UTF8_H
#define FC__FC_UTF8_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************************
  This module provides tools to make safe operations on UTF-8 strings. All
  functions are prefixed with fc_utf8_*. There are two main categories of
  functions:
  - the ones which truncate the strings at the first UTF-8 invalid
  character. They are named *_trunc();
  - the ones which attempts to replace the invalid characters by the
  replacement character (U+FFFD). They are named *_rep().
****************************************************************************/

extern const char fc_utf8_skip[256];

/* UTF-8 character functions. */
bool fc_utf8_char_validate(const char *utf8_char)
    fc__attribute((nonnull (1)));
char *fc_utf8_find_next_char(const char *utf8_char)
    fc__attribute((nonnull (1)));
char *fc_utf8_find_prev_char(const char *utf8_char, const char *utf8_string)
    fc__attribute((nonnull (1, 2)));
/* Jump to next UTF-8 character, assuming it is a valid UTF-8 string. */
#define fc_ut8_next_char(utf8_char) \
    (utf8_char + fc_utf8_skip[*(unsigned char *) utf8_char])

/* UTF-8 string functions. */
bool fc_utf8_validate(const char *utf8_string, const char **end)
    fc__attribute((nonnull (1)));
bool fc_utf8_validate_len(const char *utf8_string, size_t byte_len,
                          const char **end)
    fc__attribute((nonnull (1)));

char *fc_utf8_validate_trunc(char *utf8_string)
    fc__attribute((nonnull (1)));
char *fc_utf8_validate_trunc_len(char *utf8_string, size_t byte_len)
    fc__attribute((nonnull (1)));
char *fc_utf8_validate_trunc_dup(const char *utf8_string)
    fc__attribute((nonnull (1)))
    fc__attribute((warn_unused_result));

char *fc_utf8_validate_rep_len(char *utf8_string, size_t byte_len)
    fc__attribute((nonnull (1)));
char *fc_utf8_validate_rep_dup(const char *utf8_string)
    fc__attribute((nonnull (1)))
    fc__attribute((warn_unused_result));

size_t fc_utf8_strlen(const char *utf8_string)
    fc__attribute((nonnull (1)));

/* UTF-8-safe replacements of some string utilities. See also functions
 * defined in "support.[ch]". */
size_t fc_utf8_strlcpy_trunc(char *dest, const char *src, size_t n)
    fc__attribute((nonnull (1, 2)));
size_t fc_utf8_strlcpy_rep(char *dest, const char *src, size_t n)
    fc__attribute((nonnull (1, 2)));
size_t fc_utf8_strlcat_trunc(char *dest, const char *src, size_t n)
    fc__attribute((nonnull (1, 2)));
size_t fc_utf8_strlcat_rep(char *dest, const char *src, size_t n)
    fc__attribute((nonnull (1, 2)));

int fc_utf8_snprintf_trunc(char *str, size_t n, const char *format, ...)
    fc__attribute((__format__ (__printf__, 3, 4)))
    fc__attribute((nonnull (1, 3)));
int fc_utf8_snprintf_rep(char *str, size_t n, const char *format, ...)
    fc__attribute((__format__ (__printf__, 3, 4)))
    fc__attribute((nonnull (1, 3)));
int fc_utf8_vsnprintf_trunc(char *str, size_t n, const char *format,
                            va_list args)
    fc__attribute((nonnull (1, 3)));
int fc_utf8_vsnprintf_rep(char *str, size_t n, const char *format,
                          va_list args)
    fc__attribute((nonnull (1, 3)));
int cat_utf8_snprintf_trunc(char *str, size_t n, const char *format, ...)
    fc__attribute((__format__ (__printf__, 3, 4)))
    fc__attribute((nonnull (1, 3)));
int cat_utf8_snprintf_rep(char *str, size_t n, const char *format, ...)
    fc__attribute((__format__ (__printf__, 3, 4)))
    fc__attribute((nonnull (1, 3)));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__FC_UTF8_H */
