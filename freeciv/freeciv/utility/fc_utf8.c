/***********************************************************************
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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>
#include <string.h>

/* utility */
#include "log.h"
#include "mem.h"
#include "support.h"

#include "fc_utf8.h"


/* The length of a character for external use (at least 1 to avoid infinite
 * loops). See also fc_ut8_next_char(). */
const char fc_utf8_skip[256] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00000000 to 00001111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00010000 to 00011111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00100000 to 00101111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00110000 to 00111111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01000000 to 01001111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01010000 to 01011111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01100000 to 01101111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01110000 to 01111111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 10000000 to 10001111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 10010000 to 10011111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 10100000 to 10101111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 10110000 to 10111111. */
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 11000000 to 11001111. */
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 11010000 to 11011111. */
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* 11100000 to 11101111. */
#ifdef USE_6_BYTES_CHAR
  4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1  /* 11110000 to 11111111. */
#else
  4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1  /* 11110000 to 11111111. */
#endif /* USE_6_BYTES_CHAR */
};

/* The length of a character for internal use (0 means an invalid start of
 * a character). */
static const char fc_utf8_char_size[256] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00000000 to 00001111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00010000 to 00011111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00100000 to 00101111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00110000 to 00111111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01000000 to 01001111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01010000 to 01011111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01100000 to 01101111. */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 01110000 to 01111111. */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10000000 to 10001111. */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10010000 to 10011111. */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10100000 to 10101111. */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10110000 to 10111111. */
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 11000000 to 11001111. */
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 11010000 to 11011111. */
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* 11100000 to 11101111. */
#ifdef USE_6_BYTES_CHAR
  4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0  /* 11110000 to 11111111. */
#else
  4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0  /* 11110000 to 11111111. */
#endif /* USE_6_BYTES_CHAR */
};

#define FC_UTF8_CHAR_SIZE(utf8_char) \
  fc_utf8_char_size[*(unsigned char *) utf8_char]

#define FC_UTF8_REP_CHAR "\xef\xbf\xbd" /* U+FFFD. */


/************************************************************************//**
  Returns TRUE if the character beginning at the pointer 'utf8_char' of size
  'size' is a valid UTF-8 character.
****************************************************************************/
static inline bool base_fc_utf8_char_validate(const char *utf8_char,
                                              char size)
{
  if (1 < size) {
    do {
      utf8_char++;
      if (0x80 != (0xC0 & *(unsigned char *) utf8_char)) {
        /* Not a valid byte of the sequence. */
        return FALSE;
      }
      size--;
    } while (1 < size);
    return TRUE;
  } else {
    return (1 == size);
  }
}

/************************************************************************//**
  UTF-8-safe variant of fc_strlcpy() base function.
****************************************************************************/
static inline size_t base_fc_utf8_strlcpy_trunc(char *dest, const char *src,
                                                size_t n)
{
  const char *end;
  size_t len;

  (void) fc_utf8_validate_len(src, n, &end);
  len = end - src;
  fc_assert(len < n);
  if (0 < len) {
    memcpy(dest, src, len);
  }
  dest[len] = '\0';
  return strlen(src);
}

/************************************************************************//**
  UTF-8-safe variant of fc_strlcpy() base function.
****************************************************************************/
static inline size_t base_fc_utf8_strlcpy_rep(char *dest, const char *src,
                                              size_t n)
{
  const char *end;
  size_t src_len, len;

  fc_assert_ret_val(NULL != src, 0);

  src_len = strlen(src);
  while (TRUE) {
    if (fc_utf8_validate_len(src, n, &end)) {
      /* Valid UTF-8. */
      len = end - src;

      fc_assert(len < n);

      if (0 < len) {
        memcpy(dest, src, len);
      }
      dest[len] = '\0'; /* Valid UTF-8 string part. */
      return src_len;
    } else {
      /* '*end' is not a valid UTF-8 character. */
      len = end - src;

      fc_assert(len < n);

      if (0 < len) {
        memcpy(dest, src, len);
      }

      n -= len;
      dest += len;

      /* Try to insert the replacement character. */
      len = sizeof(FC_UTF8_REP_CHAR);
      if (n > len) {
        memcpy(dest, FC_UTF8_REP_CHAR, len);
        n -= len;
        dest += len;
      }

      if (1 == n) {
        *dest = '\0';
        return src_len; /* End of 'dest' reached. */
      }

      /* Jump to next character in src. */
      src = fc_utf8_find_next_char(end);
      if (src == NULL || *src == '\0') {
        *dest = '\0';
        return src_len; /* End of 'src' reached. */
      }
    }
  }
  fc_assert(FALSE);     /* Shouldn't occur! */
  return src_len;
}


/************************************************************************//**
  Returns TRUE if the character beginning at the pointer 'utf8_char' is
  a valid UTF-8 character.
****************************************************************************/
bool fc_utf8_char_validate(const char *utf8_char)
{
  fc_assert_ret_val(NULL != utf8_char, FALSE);

  return base_fc_utf8_char_validate(utf8_char, FC_UTF8_CHAR_SIZE(utf8_char));
}

/************************************************************************//**
  Jump to next UTF-8 character start.

  NB: This function can return a invalid UTF-8 character. Check with
  fc_utf8_char_validate() to unsure.
****************************************************************************/
char *fc_utf8_find_next_char(const char *utf8_char)
{
  fc_assert_ret_val(NULL != utf8_char, NULL);

  do {
    utf8_char++;
  } while (0 == FC_UTF8_CHAR_SIZE(utf8_char));
  return (char *) utf8_char;
}

/************************************************************************//**
  Jump to previous UTF-8 character start in the limit of the 'utf8_string'
  pointer. If no character is found, returns 'utf8_string'.

  NB: This function can return a invalid UTF-8 character. Check with
  fc_utf8_char_validate() to unsure.
****************************************************************************/
char *fc_utf8_find_prev_char(const char *utf8_char, const char *utf8_string)
{
  fc_assert_ret_val(NULL != utf8_char, NULL);

  for (utf8_char--; utf8_char > utf8_string; utf8_char--) {
    if (0 != FC_UTF8_CHAR_SIZE(utf8_char)) {
      return (char *) utf8_char;
    }
  }
  return (char *) utf8_string;
}


/************************************************************************//**
  Returns TRUE if the string 'utf8_string' contains only valid UTF-8
  characters. If 'end' is not NULL, the end of the valid string will be
  stored there, even if it returns TRUE.

  See also fc_utf8_validate_len().
****************************************************************************/
bool fc_utf8_validate(const char *utf8_string, const char **end)
{
  char size;

  fc_assert_ret_val(NULL != utf8_string, FALSE);

  while ('\0' != *utf8_string) {
    size = FC_UTF8_CHAR_SIZE(utf8_string);
    if (!base_fc_utf8_char_validate(utf8_string, size)) {
      if (NULL != end) {
        *end = utf8_string;
      }
      return FALSE;
    }
    utf8_string += size;
  }
  if (NULL != end) {
    *end = utf8_string;
  }
  return TRUE;
}

/************************************************************************//**
  Returns TRUE if the string 'utf8_string' contains only valid UTF-8
  characters in the limit of the length (in bytes) 'byte_len'. If 'end' is
  not NULL, the end of the valid string will be stored there, even if it
  returns TRUE.

  See also fc_utf8_validate().
****************************************************************************/
bool fc_utf8_validate_len(const char *utf8_string, size_t byte_len,
                          const char **end)
{
  char size;

  fc_assert_ret_val(NULL != utf8_string, FALSE);

  while ('\0' != *utf8_string) {
    size = FC_UTF8_CHAR_SIZE(utf8_string);

    if (!base_fc_utf8_char_validate(utf8_string, size)) {
      if (NULL != end) {
        *end = utf8_string;
      }
      return FALSE;
    }

    if (size > byte_len) {
      if (NULL != end) {
        *end = utf8_string;
      }
      return FALSE;
    } else {
      byte_len -= size;
    }

    utf8_string += size;
  }
  if (NULL != end) {
    *end = utf8_string;
  }
  return TRUE;
}

/************************************************************************//**
  Truncate the string 'utf8_string' at the first invalid UTF-8 character.
  Returns 'utf8_string'.

  See also fc_utf8_validate(), fc_utf8_validate_trunc_len(),
  and fc_utf8_validate_trunc_dup().
****************************************************************************/
char *fc_utf8_validate_trunc(char *utf8_string)
{
  char *end;

  fc_assert_ret_val(NULL != utf8_string, NULL);

  if (!fc_utf8_validate(utf8_string, (const char **) &end)) {
    *end = '\0';
  }
  return utf8_string;
}

/************************************************************************//**
  Truncate the string 'utf8_string' at the first invalid UTF-8 character in
  the limit (in bytes) of 'byte_len'. Returns 'utf8_string'.

  See also fc_utf8_validate_trunc(), fc_utf8_validate_trunc_dup(),
  and fc_utf8_validate_rep_len().
****************************************************************************/
char *fc_utf8_validate_trunc_len(char *utf8_string, size_t byte_len)
{
  char *end;

  fc_assert_ret_val(NULL != utf8_string, NULL);

  if (!fc_utf8_validate_len(utf8_string, byte_len, (const char **) &end)) {
    *end = '\0';
  }
  return utf8_string;
}

/************************************************************************//**
  Duplicate the truncation of the string 'utf8_string' at the first invalid
  UTF-8 character.

  See also fc_utf8_validate_trunc(), fc_utf8_validate_trunc_len(),
  and fc_utf8_validate_rep_dup().
****************************************************************************/
char *fc_utf8_validate_trunc_dup(const char *utf8_string)
{
  const char *end;
  size_t size;
  char *ret;

  fc_assert_ret_val(NULL != utf8_string, NULL);

  (void) fc_utf8_validate(utf8_string, &end);
  size = end - utf8_string;
  ret = fc_malloc(size + 1);    /* Keep a spot for '\0'. */
  memcpy(ret, utf8_string, size);
  ret[size] = '\0';

  return ret;
}

/************************************************************************//**
  Transform 'utf8_string' with replacing all invalid characters with the
  replacement character in the limit of 'byte_len', truncate the last
  character. Returns 'utf8_string'.

  See also fc_utf8_validate_len(), fc_utf8_validate_trunc(),
  and fc_utf8_validate_rep_dup().
****************************************************************************/
char *fc_utf8_validate_rep_len(char *utf8_string, size_t byte_len)
{
  fc_assert_ret_val(NULL != utf8_string, NULL);

  if (0 < byte_len) {
    char copy[byte_len];

    fc_strlcpy(copy, utf8_string, byte_len);
    base_fc_utf8_strlcpy_rep(utf8_string, copy, byte_len);
  }
  return utf8_string;
}

/************************************************************************//**
  Duplicate 'utf8_string' and replace all invalid characters with the
  replacement character.

  See also fc_utf8_validate_rep_len(), and fc_utf8_validate_trunc_dup().
****************************************************************************/
char *fc_utf8_validate_rep_dup(const char *utf8_string)
{
  char *ret;
  const char *utf8_char;
  size_t size = 1;      /* '\0'. */
  char char_size;

  fc_assert_ret_val(NULL != utf8_string, NULL);

  /* Check needed size. */
  utf8_char = utf8_string;
  while ('\0' != *utf8_char) {
    char_size = FC_UTF8_CHAR_SIZE(utf8_char);
    if (base_fc_utf8_char_validate(utf8_char, char_size)) {
      /* Normal valid character. */
      size += char_size;
      utf8_char += char_size;
    } else {
      /* Replacement character. */
      size += sizeof(FC_UTF8_REP_CHAR);
      /* Find next character. */
      do {
        utf8_char++;
      } while (0 == FC_UTF8_CHAR_SIZE(utf8_char));
    }
  }

  /* Do the allocation. */
  ret = fc_malloc(size);
  base_fc_utf8_strlcpy_rep(ret, utf8_string, size);

  return ret;
}

/************************************************************************//**
  Returns the number of characters in the string 'utf8_string'. To know the
  number of used bytes, used strlen() instead.

  NB: 'utf8_string' must be UTF-8 valid (see fc_utf8_validate()), or the
  behaviour of this function will be unknown.
****************************************************************************/
size_t fc_utf8_strlen(const char *utf8_string)
{
  size_t len;

  fc_assert_ret_val(NULL != utf8_string, 0);

  for (len = 0; '\0' != *utf8_string; len++) {
    utf8_string = fc_ut8_next_char(utf8_string);
  }
  return len;
}


/************************************************************************//**
  This is a variant of fc_strlcpy() to unsure the result will be a valid
  UTF-8 string. It truncates the string at the first UTF-8 invalid
  character.

  See also fc_strlcpy(), fc_utf8_strlcpy_rep().
****************************************************************************/
size_t fc_utf8_strlcpy_trunc(char *dest, const char *src, size_t n)
{
  fc_assert_ret_val(NULL != dest, -1);
  fc_assert_ret_val(NULL != src, -1);
  fc_assert_ret_val(0 < n, -1);

  return base_fc_utf8_strlcpy_trunc(dest, src, n);
}

/************************************************************************//**
  This is a variant of fc_strlcpy() to unsure the result will be a valid
  UTF-8 string. Unlike fc_utf8_strlcpy_trunc(), it replaces the invalid
  characters by the replacement character, instead of truncating the string.

  See also fc_strlcpy(), fc_utf8_strlcpy_trunc().
****************************************************************************/
size_t fc_utf8_strlcpy_rep(char *dest, const char *src, size_t n)
{
  fc_assert_ret_val(NULL != dest, -1);
  fc_assert_ret_val(NULL != src, -1);
  fc_assert_ret_val(0 < n, -1);

  return base_fc_utf8_strlcpy_rep(dest, src, n);
}

/************************************************************************//**
  This is a variant of fc_strlcat() to unsure the result will be a valid
  UTF-8 string. It truncates the string at the first UTF-8 invalid
  character.

  NB: This function doesn't perform anything on the already edited part of
  the string 'dest', which can contain invalid UTF-8 characters.

  See also fc_strlcat(), fc_utf8_strlcat_rep().
****************************************************************************/
size_t fc_utf8_strlcat_trunc(char *dest, const char *src, size_t n)
{
  size_t len;

  fc_assert_ret_val(NULL != dest, -1);
  fc_assert_ret_val(NULL != src, -1);
  fc_assert_ret_val(0 < n, -1);

  len = strlen(dest);
  fc_assert_ret_val(len < n, -1);
  return len + base_fc_utf8_strlcpy_trunc(dest + len, src, n - len);
}

/************************************************************************//**
  This is a variant of fc_strlcat() to unsure the result will be a valid
  UTF-8 string. Unlike fc_utf8_strlcat_trunc(), it replaces the invalid
  characters by the replacement character, instead of truncating the string.

  NB: This function doesn't perform anything on the already edited part of
  the string 'dest', which can contain invalid UTF-8 characters.

  See also fc_strlcat(), fc_utf8_strlcat_trunc().
****************************************************************************/
size_t fc_utf8_strlcat_rep(char *dest, const char *src, size_t n)
{
  size_t len;

  fc_assert_ret_val(NULL != dest, -1);
  fc_assert_ret_val(NULL != src, -1);
  fc_assert_ret_val(0 < n, -1);

  len = strlen(dest);
  fc_assert_ret_val(len < n, -1);
  return len + base_fc_utf8_strlcpy_rep(dest + len, src, n - len);
}

/************************************************************************//**
  This is a variant of fc_snprintf() to unsure the result will be a valid
  UTF-8 string. It truncates the string at the first UTF-8 invalid
  character.

  See also fc_snprintf(), fc_utf8_snprintf_rep().
****************************************************************************/
int fc_utf8_snprintf_trunc(char *str, size_t n, const char *format, ...)
{
  int ret;
  va_list args;

  va_start(args, format);
  ret = fc_utf8_vsnprintf_trunc(str, n, format, args);
  va_end(args);
  return ret;
}

/************************************************************************//**
  This is a variant of fc_snprintf() to unsure the result will be a valid
  UTF-8 string. Unlike fc_utf8_snprintf_trunc(), it replaces the invalid
  characters by the replacement character, instead of truncating the string.

  See also fc_snprintf(), fc_utf8_snprintf_trunc().
****************************************************************************/
int fc_utf8_snprintf_rep(char *str, size_t n, const char *format, ...)
{
  int ret;
  va_list args;

  va_start(args, format);
  ret = fc_utf8_vsnprintf_rep(str, n, format, args);
  va_end(args);
  return ret;
}

/************************************************************************//**
  This is a variant of fc_vsnprintf() to unsure the result will be a valid
  UTF-8 string. It truncates the string at the first UTF-8 invalid
  character.

  See also fc_vsnprintf(), fc_utf8_vsnprintf_rep().
****************************************************************************/
int fc_utf8_vsnprintf_trunc(char *str, size_t n, const char *format,
                            va_list args)
{
  char *end;
  int ret;

  fc_assert_ret_val(NULL != str, -1);
  fc_assert_ret_val(0 < n, -1);
  fc_assert_ret_val(NULL != format, -1);

  ret = fc_vsnprintf(str, n, format, args);
  if (fc_utf8_validate(str, (const char **) &end)) {
    /* Already valid UTF-8. */
    return ret;
  } else {
    /* Truncate at last valid UTF-8 character. */
    *end = '\0';
    return (-1 == ret ? -1 : end - str);
  }
}

/************************************************************************//**
  This is a variant of fc_vsnprintf() to unsure the result will be a valid
  UTF-8 string. Unlike fc_utf8_vsnprintf_trunc(), it replaces the invalid
  characters by the replacement character, instead of truncating the string.

  See also fc_vsnprintf(), fc_utf8_vsnprintf_trunc().
****************************************************************************/
int fc_utf8_vsnprintf_rep(char *str, size_t n, const char *format,
                          va_list args)
{
  char *end;
  int ret;

  fc_assert_ret_val(NULL != str, -1);
  fc_assert_ret_val(0 < n, -1);
  fc_assert_ret_val(NULL != format, -1);

  ret = fc_vsnprintf(str, n, format, args);
  if (fc_utf8_validate(str, (const char **) &end)) {
    /* Already valid UTF-8. */
    return ret;
  } else {
    (void) fc_utf8_validate_rep_len(end, n - (end - str));
    return (-1 == ret ? -1 : strlen(str));
  }
}

/************************************************************************//**
  This is a variant of cat_snprintf() to unsure the result will be a valid
  UTF-8 string. It truncates the string at the first UTF-8 invalid
  character.

  NB: This function doesn't perform anything on the already edited part of
  the string 'str', which can contain invalid UTF-8 characters.

  See also cat_snprintf(), cat_utf8_snprintf_rep().
****************************************************************************/
int cat_utf8_snprintf_trunc(char *str, size_t n, const char *format, ...)
{
  size_t len;
  int ret;
  va_list args;

  fc_assert_ret_val(NULL != format, -1);
  fc_assert_ret_val(NULL != str, -1);
  fc_assert_ret_val(0 < n, -1);

  len = strlen(str);
  fc_assert_ret_val(len < n, -1);

  va_start(args, format);
  ret = fc_utf8_vsnprintf_trunc(str + len, n - len, format, args);
  va_end(args);
  return (-1 == ret ? -1 : ret + len);
}

/************************************************************************//**
  This is a variant of cat_snprintf() to unsure the result will be a valid
  UTF-8 string. Unlike cat_utf8_snprintf_trunc(), it replaces the invalid
  characters by the replacement character, instead of truncating the string.

  NB: This function doesn't perform anything on the already edited part of
  the string 'str', which can contain invalid UTF-8 characters.

  See also cat_snprintf(), cat_utf8_snprintf_trunc().
****************************************************************************/
int cat_utf8_snprintf_rep(char *str, size_t n, const char *format, ...)
{
  size_t len;
  int ret;
  va_list args;

  fc_assert_ret_val(NULL != format, -1);
  fc_assert_ret_val(NULL != str, -1);
  fc_assert_ret_val(0 < n, -1);

  len = strlen(str);
  fc_assert_ret_val(len < n, -1);

  va_start(args, format);
  ret = fc_utf8_vsnprintf_rep(str + len, n - len, format, args);
  va_end(args);
  return (-1 == ret ? -1 : ret + len);
}
