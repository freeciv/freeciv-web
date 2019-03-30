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

/***********************************************************************
  This module contains replacements for functions which are not
  available on all platforms.  Where the functions are available
  natively, these are (mostly) just wrappers.

  Notice the function names here are prefixed by, eg, "fc".  An
  alternative would be to use the "standard" function name, and
  provide the implementation only if required.  However the method
  here has some advantages:

   - We can provide definite prototypes in support.h, rather than
   worrying about whether a system prototype exists, and if so where,
   and whether it is correct.  (Note that whether or not configure
   finds a function and defines HAVE_FOO does not necessarily say
   whether or not there is a _prototype_ for the function available.)

   - We don't have to include fc_config.h in support.h, but can instead
   restrict it to this .c file.

   - We can add some extra stuff to these functions if we want.

  The main disadvantage is remembering to use these "fc" functions on
  systems which have the functions natively.

***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "fc_prehdrs.h"

#include <ctype.h>
#include <errno.h>
#include <math.h> /* ceil() */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef GENERATING_MAC
#include <events.h>		/* for WaitNextEvent() */
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>		/* usleep, fcntl, gethostname */
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef FREECIV_HAVE_LIBZ
#include <zlib.h>
#endif
#ifdef FREECIV_MSWINDOWS
#include <process.h>
#include <windows.h>
#endif /* FREECIV_MSWINDOWS */
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif
#ifdef HAVE_LIBGEN_H
/* POSIX version of basename() */
#  include <libgen.h>
#endif

#ifdef FREECIV_HAVE_LIBZ
#include <zlib.h>
#endif

/* ICU */
#include "unicode/ustring.h"

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"

#include "support.h"

static int icu_buffer_uchars = 0;
static UChar *icu_buffer1 = NULL;
static UChar *icu_buffer2 = NULL;
fc_mutex icu_buffer_mutex;

/************************************************************************//**
  Initial allocation of string comparison buffers.
****************************************************************************/
static void icu_buffers_initial(void)
{
  if (icu_buffer1 == NULL) {
    icu_buffer_uchars = 1024;
    icu_buffer1 = fc_malloc((icu_buffer_uchars + 1) * sizeof(UChar));
    icu_buffer2 = fc_malloc((icu_buffer_uchars + 1) * sizeof(UChar));

    /* Make sure there's zero after the buffer published with cmp_buffer_uchars */
    icu_buffer1[icu_buffer_uchars] = 0;
    icu_buffer2[icu_buffer_uchars] = 0;
  }
}

/************************************************************************//**
  Make string comparison buffers bigger
****************************************************************************/
static void icu_buffers_increase(void)
{
  icu_buffer_uchars *= 1.5;
  icu_buffer1 = fc_realloc(icu_buffer1, (icu_buffer_uchars + 1) * sizeof(UChar));
  icu_buffer2 = fc_realloc(icu_buffer2, (icu_buffer_uchars + 1) * sizeof(UChar));

  /* Make sure there's zero after the buffer published with cmp_buffer_uchars */
  icu_buffer1[icu_buffer_uchars] = 0;
  icu_buffer2[icu_buffer_uchars] = 0;
}

/************************************************************************//**
  Initialize string handling API
****************************************************************************/
void fc_strAPI_init(void)
{
  if (icu_buffer_uchars == 0) {
    fc_init_mutex(&icu_buffer_mutex);
    icu_buffers_initial();
  }
}

/************************************************************************//**
  Free string handling API resources
****************************************************************************/
void fc_strAPI_free(void)
{
  if (icu_buffer1 != NULL) {
    free(icu_buffer1);
    icu_buffer1 = NULL;
    free(icu_buffer2);
    icu_buffer2 = NULL;
    icu_buffer_uchars = 0;
  }
  fc_destroy_mutex(&icu_buffer_mutex);
}

/************************************************************************//**
  Compare strings like strcmp(), but ignoring case.
****************************************************************************/
int fc_strcasecmp(const char *str0, const char *str1)
{
  UErrorCode err_code = U_ZERO_ERROR;
  int len0;
  int len1;
  bool enough_mem = FALSE;
  int ret;

  if (str0 == NULL) {
    return -1;
  }
  if (str1 == NULL) {
    return 1;
  }

  if (icu_buffer_uchars == 0) {
    fc_strAPI_init();
  }

  fc_allocate_mutex(&icu_buffer_mutex);

  while (!enough_mem) {
    UErrorCode err_code0 = U_ZERO_ERROR;
    UErrorCode err_code1 = U_ZERO_ERROR;

    u_strFromUTF8Lenient(icu_buffer1, icu_buffer_uchars, &len0, str0, -1, &err_code0);
    u_strFromUTF8Lenient(icu_buffer2, icu_buffer_uchars, &len1, str1, -1, &err_code1);

    /* No need to handle U_STRING_NOT_TERMINATED_WARNING here as there's '0' after
     * the buffers we were using */
    if (err_code0 == U_BUFFER_OVERFLOW_ERROR || err_code1 == U_BUFFER_OVERFLOW_ERROR) {
      icu_buffers_increase();
    } else {
      enough_mem = TRUE;
    }
  }

  ret = u_strCaseCompare(icu_buffer1, -1, icu_buffer2, -1,
                         0, &err_code);

  fc_release_mutex(&icu_buffer_mutex);

  return ret;
}

/************************************************************************//**
  Compare strings like strncmp(), but ignoring case.
  ie, only compares first n chars.
****************************************************************************/
int fc_strncasecmp(const char *str0, const char *str1, size_t n)
{
  UErrorCode err_code = U_ZERO_ERROR;
  int len0;
  int len1;
  bool enough_mem = FALSE;
  int ret;

  if (str0 == NULL) {
    return -1;
  }
  if (str1 == NULL) {
    return 1;
  }

  if (icu_buffer_uchars == 0) {
    fc_strAPI_init();
  }

  fc_allocate_mutex(&icu_buffer_mutex);

  while (!enough_mem) {
    UErrorCode err_code0 = U_ZERO_ERROR;
    UErrorCode err_code1 = U_ZERO_ERROR;

    u_strFromUTF8Lenient(icu_buffer1, icu_buffer_uchars, &len0, str0, -1, &err_code0);
    u_strFromUTF8Lenient(icu_buffer2, icu_buffer_uchars, &len1, str1, -1, &err_code1);

    /* No need to handle U_STRING_NOT_TERMINATED_WARNING here as there's '0' after
     * the buffers we were using */
    if (err_code0 == U_BUFFER_OVERFLOW_ERROR || err_code1 == U_BUFFER_OVERFLOW_ERROR) {
      icu_buffers_increase();
    } else {
      enough_mem = TRUE;
    }
  }

  if (len0 > n) {
    len0 = n;
  }
  if (len1 > n) {
    len1 = n;
  }

  ret = u_strCaseCompare(icu_buffer1, len0, icu_buffer2, len1,
                         0, &err_code);

  fc_release_mutex(&icu_buffer_mutex);

  return ret;
}

/************************************************************************//**
  Copies a string and convert the following characters:
  - '\n' to "\\n".
  - '\\' to "\\\\".
  - '\"' to "\\\"".
  See also remove_escapes().
****************************************************************************/
void make_escapes(const char *str, char *buf, size_t buf_len)
{
  char *dest = buf;
  /* Sometimes we insert 2 characters at once ('\n' -> "\\n"), so keep
   * place for '\0' and an extra character. */
  const char *const max = buf + buf_len - 2;

  while (*str != '\0' && dest < max) {
    switch (*str) {
    case '\n':
      *dest++ = '\\';
      *dest++ = 'n';
      str++;
      break;
    case '\\':
    case '\"':
      *dest++ = '\\';
      /* Fallthrough. */
    default:
      *dest++ = *str++;
      break;
    }
  }
  *dest = 0;
}

/************************************************************************//**
  Copies a string. Backslash followed by a genuine newline always
  removes the newline.
  If full_escapes is TRUE:
    - '\n' -> newline translation.
    - Other '\c' sequences (any character 'c') are just passed
      through with the '\' removed (eg, includes '\\', '\"').
  See also make_escapes().
****************************************************************************/
void remove_escapes(const char *str, bool full_escapes,
                    char *buf, size_t buf_len)
{
  char *dest = buf;
  const char *const max = buf + buf_len - 1;

  while (*str != '\0' && dest < max) {
    if (*str == '\\' && *(str + 1) == '\n') {
      /* Escape followed by newline. Skip both */
      str += 2;
    } else if (full_escapes && *str == '\\') {
      str++;
      if (*str == 'n') {
        *dest++ = '\n';
        str++;
      }
    } else {
      *dest++ = *str++;
    }
  }
  *dest = '\0';
}

/************************************************************************//**
  Count length of string without possible surrounding quotes.
****************************************************************************/
size_t effectivestrlenquote(const char *str)
{
  int len;
  if (!str) {
    return 0;
  }

  len = strlen(str);

  if (str[0] == '"' && str[len-1] == '"') {
    return len - 2;
  }

  return len;
}

/************************************************************************//**
  Compare strings like strncasecmp() but ignoring surrounding
  quotes in either string.
****************************************************************************/
int fc_strncasequotecmp(const char *str0, const char *str1, size_t n)
{
  size_t i;
  size_t len0;
  size_t len1;
  size_t cmplen;

  if (str0 == NULL) {
    return -1;
  }
  if (str1 == NULL) {
    return 1;
  }

  len0 = strlen(str0); /* TODO: We iterate string once already here, */
  len1 = strlen(str1); /*       could iterate only once */

  if (str0[0] == '"') {
    if (str0[len0 - 1] == '"') {
      /* Surrounded with quotes */
      str0++;
      len0 -= 2;
    }
  }

  if (str1[0] == '"') {
    if (str1[len1 - 1] == '"') {
      /* Surrounded with quotes */
      str1++;
      len1 -= 2;
    }
  }

  if (len0 < n || len1 < n) {
    /* One of the strings is shorter than what should be compared... */
    if (len0 != len1) {
      /* ...and another is longer than it. */
      return len0 - len1;
    }

    cmplen = len0; /* This avoids comparing ending quote */
  } else {
    cmplen = n;
  }

  for (i = 0; i < cmplen ; i++, str0++, str1++) {
    if (fc_tolower(*str0) != fc_tolower(*str1)) {
      return ((int) (unsigned char) fc_tolower(*str0))
             - ((int) (unsigned char) fc_tolower(*str1));
    }
  }

  /* All characters compared and all matched */
  return 0;
}

/************************************************************************//**
  Return the needle in the haystack (or NULL).
  Naive implementation.
****************************************************************************/
char *fc_strcasestr(const char *haystack, const char *needle)
{
#ifdef HAVE_STRCASESTR
  return strcasestr(haystack, needle);
#else
  size_t haystacks;
  size_t needles;
  const char *p;

  if (NULL == needle || '\0' == *needle) {
    return (char *)haystack;
  }
  if (NULL == haystack || '\0' == *haystack) {
    return NULL;
  }
  haystacks = strlen(haystack);
  needles = strlen(needle);
  if (haystacks < needles) {
    return NULL;
  }

  for (p = haystack; p <= &haystack[haystacks - needles]; p++) {
    if (0 == fc_strncasecmp(p, needle, needles)) {
      return (char *)p;
    }
  }
  return NULL;
#endif /* HAVE_STRCASESTR */
}

/************************************************************************//**
  Wrapper function for strcoll().
****************************************************************************/
int fc_strcoll(const char *str0, const char *str1)
{
#if defined(ENABLE_NLS) && defined(HAVE_STRCOLL)
  return strcoll(str0, str1);
#elif defined(ENABLE_NLS) && defined(HAVE__STRCOLL)
  return _strcoll(str0, str1);
#else
  return strcmp(str0, str1);
#endif
}

/************************************************************************//**
  Wrapper function for stricoll().
****************************************************************************/
int fc_stricoll(const char *str0, const char *str1)
{
  /* We prefer _stricoll() over stricoll() since
   * latter is not declared in MinGW headers causing compiler
   * warning, preventing -Werror builds. */
#if defined(ENABLE_NLS) && defined(HAVE__STRICOLL)
  return _stricoll(str0, str1);
#elif defined(ENABLE_NLS) && defined(HAVE_STRICOLL)
  return stricoll(str0, str1);
#elif defined(ENABLE_NLS) && defined(HAVE_STRCASECOLL)
  return strcasecoll(str0, str1);
#else
  return fc_strcasecmp(str0, str1);
#endif
}

/************************************************************************//**
  Wrapper function for fopen() with filename conversion to local
  encoding on Windows.
****************************************************************************/
FILE *fc_fopen(const char *filename, const char *opentype)
{
#ifdef FREECIV_MSWINDOWS
  FILE *result;
  char *filename_in_local_encoding =
    internal_to_local_string_malloc(filename);

  result = fopen(filename_in_local_encoding, opentype);
  free(filename_in_local_encoding);
  return result;
#else  /* FREECIV_MSWINDOWS */
  return fopen(filename, opentype);
#endif /* FREECIV_MSWINDOWS */
}

/************************************************************************//**
  Wrapper function for gzopen() with filename conversion to local
  encoding on Windows.
****************************************************************************/
#ifdef FREECIV_HAVE_LIBZ
gzFile fc_gzopen(const char *filename, const char *opentype)
{
#ifdef FREECIV_MSWINDOWS
  gzFile result;
  char *filename_in_local_encoding =
    internal_to_local_string_malloc(filename);

  result = gzopen(filename_in_local_encoding, opentype);
  free(filename_in_local_encoding);
  return result;
#else  /* FREECIV_MSWINDOWS */
  return gzopen(filename, opentype);
#endif /* FREECIV_MSWINDOWS */
}
#endif /* FREECIV_HAVE_LIBZ */

/************************************************************************//**
  Wrapper function for remove() with filename conversion to local
  encoding on Windows.
****************************************************************************/
int fc_remove(const char *filename)
{
#ifdef FREECIV_MSWINDOWS
  int result;
  char *filename_in_local_encoding =
    internal_to_local_string_malloc(filename);

  result = remove(filename_in_local_encoding);
  free(filename_in_local_encoding);
  return result;
#else  /* FREECIV_MSWINDOWS */
  return remove(filename);
#endif /* FREECIV_MSWINDOWS */
}

/************************************************************************//**
  Wrapper function for stat() with filename conversion to local
  encoding on Windows.
****************************************************************************/
int fc_stat(const char *filename, struct stat *buf)
{
#ifdef FREECIV_MSWINDOWS
  int result;
  char *filename_in_local_encoding =
    internal_to_local_string_malloc(filename);

  result = stat(filename_in_local_encoding, buf);
  free(filename_in_local_encoding);
  return result;
#else  /* FREECIV_MSWINDOWS */
  return stat(filename, buf);
#endif /* FREECIV_MSWINDOWS */
}

/************************************************************************//**
  Returns last error code.
****************************************************************************/
fc_errno fc_get_errno(void)
{
#ifdef FREECIV_MSWINDOWS
  return GetLastError();
#else  /* FREECIV_MSWINDOWS */
  return errno;
#endif /* FREECIV_MSWINDOWS */
}

/************************************************************************//**
  Return a string which describes a given error (errno-style.)
  The string is converted as necessary from the local_encoding
  to internal_encoding, for inclusion in translations.  May be
  subsequently converted back to local_encoding for display.

  Note that this is not the reentrant form.
****************************************************************************/
const char *fc_strerror(fc_errno err)
{
#ifdef FREECIV_MSWINDOWS
  static char buf[256];

  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		     NULL, err, 0, buf, sizeof(buf), NULL)) {
    fc_snprintf(buf, sizeof(buf),
		_("error %ld (failed FormatMessage)"), err);
  }
  return buf;
#else  /* FREECIV_MSWINDOWS */
#ifdef HAVE_STRERROR
  static char buf[256];

  return local_to_internal_string_buffer(strerror(err),
                                         buf, sizeof(buf));
#else  /* HAVE_STRERROR */
  static char buf[64];

  fc_snprintf(buf, sizeof(buf),
	      _("error %d (compiled without strerror)"), err);
  return buf;
#endif /* HAVE_STRERROR */
#endif /* FREECIV_MSWINDOWS */
}

/************************************************************************//**
  Suspend execution for the specified number of microseconds.
****************************************************************************/
void fc_usleep(unsigned long usec)
{
#ifdef HAVE_USLEEP
  usleep(usec);
#else  /* HAVE_USLEEP */
#ifdef HAVE_SNOOZE		/* BeOS */
  snooze(usec);
#else  /* HAVE_SNOOZE */
#ifdef GENERATING_MAC
  EventRecord the_event;	/* dummy - always be a null event */

  usec /= 16666;		/* microseconds to 1/60th seconds */
  if (usec < 1) {
    usec = 1;
  }
  /* supposed to give other application processor time for the mac */
  WaitNextEvent(0, &the_event, usec, 0L);
#else  /* GENERATING_MAC */
#ifdef FREECIV_MSWINDOWS
  Sleep(usec / 1000);
#else  /* FREECIV_MSWINDOWS */
  fc_timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = usec;
  /* FIXME: an interrupt can cause an EINTR return here.  In that case we
   * need to have another select call. */
  fc_select(0, NULL, NULL, NULL, &tv);
#endif /* FREECIV_MSWINDOWS */
#endif /* GENERATING_MAC */
#endif /* HAVE_SNOOZE */
#endif /* HAVE_USLEEP */
}

/************************************************************************//**
  Replace 'search' by 'replace' within 'str'. If needed 'str' is resized
  using realloc() to fit the modified string. The new pointer to the string
  is returned.
****************************************************************************/
char *fc_strrep_resize(char *str, size_t *len, const char *search,
                       const char *replace)
{
  size_t len_max;
  bool success;

  fc_assert_ret_val(str != NULL, NULL);
  fc_assert_ret_val(len != NULL, NULL);
  if (search == NULL || replace == NULL) {
    return str;
  }

  len_max = ceil((double)strlen(str) * strlen(replace) / strlen(search)) + 1;
  if ((*len) < len_max) {
    /* replace string is longer than search string; allocated enough memory
     * for the worst case */
    (*len) = len_max;
    str = fc_realloc(str, len_max);
  }

  success = fc_strrep(str, (*len), search, replace);
  /* should never happen */
  fc_assert_ret_val_msg(success == TRUE, NULL,
                        "Can't replace '%s' by '%s' in '%s'. To small "
                        "size after reallocation: %lu.", search, replace,
                        str, (long unsigned int)*len);

  return str;
}

/************************************************************************//**
  Replace 'search' by 'replace' within 'str'. sizeof(str) should be large
  enough for the modified value of 'str'. Returns TRUE if the replacement
  was successful.
****************************************************************************/
bool fc_strrep(char *str, size_t len, const char *search,
               const char *replace)
{
  size_t len_search, len_replace;
  char *s, *p;

  fc_assert_ret_val(str != NULL, FALSE);
  if (search == NULL || replace == NULL) {
    return TRUE;
  }

  len_search = strlen(search);
  len_replace = strlen(replace);

  s = str;
  while (s != NULL) {
    p = strstr(s, search);
    if (p == NULL) {
      /* nothing found */
      break;
    }

    if (len < (strlen(str) + len_replace - len_search + 1)) {
      /* sizeof(str) not large enough to do the replacement */
      return FALSE;
    }

    memmove(p + len_replace, p + len_search, strlen(p + len_search) + 1);
    memcpy(p, replace, len_replace);
    s = p + len_replace;
  }

  return TRUE;
}

/************************************************************************//**
  fc_strlcpy() provides utf-8 version of (non-standard) function strlcpy()
  It is intended as more user-friendly version of strncpy(), in particular
  easier to use safely and correctly, and ensuring nul-terminated results
  while being able to detect truncation.

  n is the full size of the destination buffer, including
  space for trailing nul, and including the pre-existing
  string for fc_strlcat().  Thus can eg use sizeof(buffer),
  or exact size malloc-ed.

  Result is always nul-terminated, whether or not truncation occurs,
  and the return value is the strlen the destination would have had
  without truncation.  I.e., a return value >= input n indicates
  truncation occurred.

  Not sure about the asserts below, but they are easier than
  trying to ensure correct behaviour on strange inputs.
  In particular note that n == 0 is prohibited (e.g., since there
  must at least be room for a nul); could consider other options.
****************************************************************************/
size_t fc_strlcpy(char *dest, const char *src, size_t n)
{
  bool enough_mem = FALSE;
  int slen;
  int dlen;
  UErrorCode err_code = U_ZERO_ERROR;

  fc_assert_ret_val(NULL != dest, -1);
  fc_assert_ret_val(NULL != src, -1);
  fc_assert_ret_val(0 < n, -1);

  if (icu_buffer_uchars == 0) {
    fc_strAPI_init();
  }

  fc_allocate_mutex(&icu_buffer_mutex);

  while (!enough_mem) {
    u_strFromUTF8(icu_buffer1, icu_buffer_uchars, &slen, src, -1, &err_code);

    /* No need to handle U_STRING_NOT_TERMINATED_WARNING here as there's '0' after
     * the buffers we were using */
    if (err_code == U_BUFFER_OVERFLOW_ERROR) {
      icu_buffers_increase();
      err_code = U_ZERO_ERROR;
    } else {
      enough_mem = TRUE;
    }
  }

  u_strToUTF8(dest, n - 1, &dlen, icu_buffer1, slen, &err_code);

  fc_release_mutex(&icu_buffer_mutex);

  dest[n - 1] = '\0';

  return dlen;
}

/************************************************************************//**
  fc_strlcat() provides utf-8 version of (non-standard) function strlcat()
  It is intended as more user-friendly version of strncat(), in particular
  easier to use safely and correctly, and ensuring nul-terminated results
  while being able to detect truncation.
****************************************************************************/
size_t fc_strlcat(char *dest, const char *src, size_t n)
{
  size_t start;

  start = strlen(dest);

  fc_assert(start < n);

  return fc_strlcpy(dest + start, src, n - start) + start;
}

/************************************************************************//**
  vsnprintf() replacement using a big malloc()ed internal buffer,
  originally by David Pfitzner <dwp@mso.anu.edu.au>

  Parameter n specifies the maximum number of characters to produce.
  This includes the trailing null, so n should be the actual number
  of characters allocated (or sizeof for char array).  If truncation
  occurs, the result will still be null-terminated.  (I'm not sure
  whether all native vsnprintf() functions null-terminate on
  truncation; this does so even if calls native function.)

  Return value: if there is no truncation, returns the number of
  characters printed, not including the trailing null.  If truncation
  does occur, returns the number of characters which would have been
  produced without truncation.
  (Linux man page says returns -1 on truncation, but glibc seems to
  do as above nevertheless; check_native_vsnprintf() above tests this.)

  [glibc is correct.  Viz.

  PRINTF(3)           Linux Programmer's Manual           PRINTF(3)

  (Thus until glibc 2.0.6.  Since glibc 2.1 these functions follow the
  C99 standard and return the number of characters (excluding the
  trailing '\0') which would have been written to the final string if
  enough space had been available.)]

  The method is simply to malloc (first time called) a big internal
  buffer, longer than any result is likely to be (for non-malicious
  usage), then vsprintf to that buffer, and copy the appropriate
  number of characters to the destination.  Thus, this is not 100%
  safe.  But somewhat safe, and at least safer than using raw snprintf!
  :-) (And of course if you have the native version it is safe.)

  Before rushing to provide a 100% safe replacement version, consider
  the following advantages of this method:

  - It is very simple, so not likely to have many bugs (other than
  arguably the core design bug regarding absolute safety), nor need
  maintenance.

  - It uses native vsprintf() (which is required), thus exactly
  duplicates the native format-string parsing/conversions.

  - It is *very* portable.  Eg, it does not require mprotect(), nor
  does it do any of its own parsing of the format string, nor use
  any tricks to go through the va_list twice.

  See also fc_utf8_vsnprintf_trunc(), fc_utf8_vsnprintf_rep().
****************************************************************************/

/* "64k should be big enough for anyone" ;-) */
#define VSNP_BUF_SIZE (64*1024)
int fc_vsnprintf(char *str, size_t n, const char *format, va_list ap)
{
#ifdef HAVE_WORKING_VSNPRINTF
  int r;
#endif

  /* This may be overzealous, but I suspect any triggering of these to
   * be bugs.  */

  fc_assert_ret_val(NULL != str, -1);
  fc_assert_ret_val(0 < n, -1);
  fc_assert_ret_val(NULL != format, -1);

#ifdef HAVE_WORKING_VSNPRINTF
  r = vsnprintf(str, n, format, ap);
  str[n - 1] = 0;

  /* Convert C99 return value to C89.  */
  if (r >= n) {
    return -1;
  }

  return r;
#else  /* HAVE_WORKING_VSNPRINTF */
  {
    /* Don't use fc_malloc() or log_*() here, since they may call
       fc_vsnprintf() if it fails.  */
 
    static char *buf;
    size_t len;

    if (!buf) {
      buf = malloc(VSNP_BUF_SIZE);

      if (!buf) {
	fprintf(stderr, "Could not allocate %i bytes for vsnprintf() "
		"replacement.", VSNP_BUF_SIZE);
	exit(EXIT_FAILURE);
      }
    }
#ifdef HAVE_VSNPRINTF
    vsnprintf(buf, n, format, ap);
#else
    vsprintf(buf, format, ap);
#endif /* HAVE_VSNPRINTF */
    buf[VSNP_BUF_SIZE - 1] = '\0';
    len = strlen(buf);

    if (len >= VSNP_BUF_SIZE - 1) {
      fprintf(stderr, "Overflow in vsnprintf replacement!"
              " (buffer size %d) aborting...\n", VSNP_BUF_SIZE);
      abort();
    }
    if (n >= len + 1) {
      memcpy(str, buf, len+1);
      return len;
    } else {
      memcpy(str, buf, n-1);
      str[n - 1] = '\0';
      return -1;
    }
  }
#endif /* HAVE_WORKING_VSNPRINTF */
}

/************************************************************************//**
  See also fc_utf8_snprintf_trunc(), fc_utf8_snprintf_rep().
****************************************************************************/
int fc_snprintf(char *str, size_t n, const char *format, ...)
{
  int ret;
  va_list ap;

  fc_assert_ret_val(NULL != format, -1);

  va_start(ap, format);
  ret = fc_vsnprintf(str, n, format, ap);
  va_end(ap);
  return ret;
}

/************************************************************************//**
  cat_snprintf is like a combination of fc_snprintf and fc_strlcat;
  it does snprintf to the end of an existing string.

  Like fc_strlcat, n is the total length available for str, including
  existing contents and trailing nul.  If there is no extra room
  available in str, does not change the string.

  Also like fc_strlcat, returns the final length that str would have
  had without truncation, or -1 if the end of the buffer is reached.
  I.e., if return is >= n, truncation occurred.

  See also cat_utf8_snprintf(), cat_utf8_snprintf_rep().
****************************************************************************/
int cat_snprintf(char *str, size_t n, const char *format, ...)
{
  size_t len;
  int ret;
  va_list ap;

  fc_assert_ret_val(NULL != format, -1);
  fc_assert_ret_val(NULL != str, -1);
  fc_assert_ret_val(0 < n, -1);

  len = strlen(str);
  fc_assert_ret_val(len < n, -1);

  va_start(ap, format);
  ret = fc_vsnprintf(str+len, n-len, format, ap);
  va_end(ap);
  return (-1 == ret ? -1 : ret + len);
}

/************************************************************************//**
  Call gethostname() if supported, else just returns -1.
****************************************************************************/
int fc_gethostname(char *buf, size_t len)
{
#ifdef HAVE_GETHOSTNAME
  return gethostname(buf, len);
#else
  return -1;
#endif
}

#ifdef FREECIV_SOCKET_ZERO_NOT_STDIN
/****************************************************************************
  Support for console I/O in case FREECIV_SOCKET_ZERO_NOT_STDIN.
****************************************************************************/

#define CONSOLE_BUF_SIZE 100
static char console_buf[CONSOLE_BUF_SIZE + 1];

/***************************************************************************/

#ifdef FREECIV_MSWINDOWS
static HANDLE console_thread = INVALID_HANDLE_VALUE;

static DWORD WINAPI thread_proc(LPVOID arg)
{
  if (fgets(console_buf, CONSOLE_BUF_SIZE, stdin)) {
    char *s;

    if ((s = strchr(console_buf, '\n'))) {
      *s = '\0';
    }
  }

  return 0;
}
#endif /* FREECIV_MSWINDOWS */

/************************************************************************//**
  Initialize console I/O in case FREECIV_SOCKET_ZERO_NOT_STDIN.
****************************************************************************/
void fc_init_console(void)
{
#ifdef FREECIV_MSWINDOWS
  DWORD threadid;

  if (console_thread != INVALID_HANDLE_VALUE) {
    return;
  }

  console_buf[0] = '\0';
  console_thread = (HANDLE) CreateThread(NULL, 0, thread_proc, NULL, 0, &threadid);
#else  /* FREECIV_MSWINDOWS */
  static bool initialized = FALSE;

  if (!initialized) {
    initialized = TRUE;
#ifdef HAVE_FILENO
    fc_nonblock(fileno(stdin));
#endif
  }
#endif /* FREECIV_MSWINDOWS */
}

/************************************************************************//**
  Read a line from console I/O in case FREECIV_SOCKET_ZERO_NOT_STDIN.

  This returns a pointer to a statically allocated buffer.
  Subsequent calls to fc_read_console() or fc_init_console() will
  overwrite it.
****************************************************************************/
char *fc_read_console(void)
{
#ifdef FREECIV_MSWINDOWS
  if (WaitForSingleObject(console_thread, 0) == WAIT_OBJECT_0) {
    CloseHandle(console_thread);
    console_thread = INVALID_HANDLE_VALUE;

    return console_buf;
  }

  return NULL;
#else  /* FREECIV_MSWINDOWS */
  if (!feof(stdin)) {    /* input from server operator */
    static char *bufptr = console_buf;

    /* fetch chars until \n, or run out of space in buffer */
    /* blocks if fc_nonblock() in fc_init_console() failed */
    while ((*bufptr = fgetc(stdin)) != EOF) {
      if (*bufptr == '\n') {
        *bufptr = '\0';
      }
      if (*bufptr == '\0') {
	bufptr = console_buf;

	return console_buf;
      }
      if ((bufptr - console_buf) <= CONSOLE_BUF_SIZE) {
        bufptr++; /* prevent overrun */
      }
    }
  }

  return NULL;
#endif /* FREECIV_MSWINDOWS */
}

#endif /* FREECIV_SOCKET_ZERO_NOT_STDIN */

/************************************************************************//**
  Returns TRUE iff the file is a regular file or a link to a regular
  file or write_access is TRUE and the file doesn't exists yet.
****************************************************************************/
bool is_reg_file_for_access(const char *name, bool write_access)
{
  struct stat tmp;

  if (fc_stat(name, &tmp) == 0) {
    return S_ISREG(tmp.st_mode);
  } else {
    return write_access && errno == ENOENT;
  }
}

/************************************************************************//**
  Replace the spaces by line breaks when the line lenght is over the desired
  one. 'str' is modified. Returns number of lines in modified s.
****************************************************************************/
int fc_break_lines(char *str, size_t desired_len)
{
  size_t slen = (size_t)strlen(str);
  int num_lines = 0;

  /* At top of this loop, s points to the rest of string,
   * either at start or after inserted newline: */
 top:
  if (str && *str != '\0' && slen > desired_len) {
    char *c;

    num_lines++;

    /* check if there is already a newline: */
    for (c = str; c < str + desired_len; c++) {
      if (*c == '\n') {
        slen -= c + 1 - str;
        str = c + 1;
        goto top;
      }
    }

    /* find space and break: */
    for (c = str + desired_len; c > str; c--) {
      if (fc_isspace(*c)) {
        *c = '\n';
        slen -= c + 1 - str;
        str = c + 1;
        goto top;
      }
    }

    /* couldn't find a good break; settle for a bad one... */
    for (c = str + desired_len + 1; *c != '\0'; c++) {
      if (fc_isspace(*c)) {
        *c = '\n';
        slen -= c + 1 - str;
        str = c + 1;
        goto top;
      }
    }
  }

  return num_lines;
}

/****************************************************************************
  Character function wrappers

  These functions are wrappers for the libc character class functions,
  without any locale-dependent behavior. The character functions work as
  documented for ASCII. Bytes outside of the ASCII set will not be reported
  to belong to any character class, and will be left unchanged by
  transformations. This behavior is safe but not strictly correct
  forsingle-byte 8-bit- or UTF-8 encoded text; in UTF-8, any byte that is
  part of a multibyte sequence is non-ASCII.
****************************************************************************/

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
bool fc_isalnum(char c)
{
  if (128 <= (unsigned char) c) {
    return FALSE;
  }
  return isalnum((int) ((unsigned char) c)) != 0;
}

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
bool fc_isalpha(char c)
{
  if (128 <= (unsigned char) c) {
    return FALSE;
  }
  return isalpha((int) ((unsigned char) c)) != 0;
}

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
bool fc_isdigit(char c)
{
  if (128 <= (unsigned char) c) {
    return FALSE;
  }
  return isdigit((int) ((unsigned char) c)) != 0;
}

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
bool fc_isprint(char c)
{
  if (128 <= (unsigned char) c) {
    return FALSE;
  }
  return isprint((int) ((unsigned char) c)) != 0;
}

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
bool fc_isspace(char c)
{
  if (128 <= (unsigned char) c) {
    return FALSE;
  }
  return isspace((int) ((unsigned char) c)) != 0;
}

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
bool fc_isupper(char c)
{
  if (128 <= (unsigned char) c) {
    return FALSE;
  }
  return isupper((int) ((unsigned char) c)) != 0;
}

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
char fc_toupper(char c)
{
  if (128 <= (unsigned char) c) {
    return c;
  }
  return (char) toupper((int) ((unsigned char) c));
}

/************************************************************************//**
  Wrapper function to work around broken libc implementations. See above.
****************************************************************************/
char fc_tolower(char c)
{
  if (128 <= (unsigned char) c) {
    return c;
  }
  return (char) tolower((int) ((unsigned char) c));
}

/************************************************************************//**
  basename() replacement that always takes const parameter.
  POSIX basename() modifies its parameter, GNU one does not.
  Ideally we would like to use GNU one, when available, directly
  without extra string copies.
****************************************************************************/
const char *fc_basename(const char *path)
{
  static char buf[2048];

  /* Copy const parameter string to buffer that basename() can
   * modify */
  fc_strlcpy(buf, path, sizeof(buf));

  return basename(buf);
}

/************************************************************************//**
  Set quick_exit() callback if possible.
****************************************************************************/
int fc_at_quick_exit(void (*func)(void))
{
#ifdef HAVE_AT_QUICK_EXIT
  return at_quick_exit(func);
#else  /* HAVE_AT_QUICK_EXIT */
  return -1;
#endif /* HAVE_AT_QUICK_EXIT */
}
