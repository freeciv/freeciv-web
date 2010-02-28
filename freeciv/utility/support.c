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

/********************************************************************** 
  This module contains replacements for functions which are not
  available on all platforms.  Where the functions are available
  natively, these are (mostly) just wrappers.

  Notice the function names here are prefixed by, eg, "my".  An
  alternative would be to use the "standard" function name, and
  provide the implementation only if required.  However the method
  here has some advantages:
  
   - We can provide definite prototypes in support.h, rather than
   worrying about whether a system prototype exists, and if so where,
   and whether it is correct.  (Note that whether or not configure
   finds a function and defines HAVE_FOO does not necessarily say
   whether or not there is a _prototype_ for the function available.)

   - We don't have to include config.h in support.h, but can instead
   restrict it to this .c file.

   - We can add some extra stuff to these functions if we want.

  The main disadvantage is remembering to use these "my" functions on
  systems which have the functions natively.

**********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
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
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>		/* usleep, fcntl, gethostname */
#endif
#ifdef WIN32_NATIVE
#include <process.h>
#include <windows.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif

#include "fciconv.h"
#include "fcintl.h"
#include "mem.h"
#include "netintf.h"

#include "support.h"

/***************************************************************
  Compare strings like strcmp(), but ignoring case.
***************************************************************/
int mystrcasecmp(const char *str0, const char *str1)
{
  if (str0 == NULL) {
    return -1;
  }
  if (str1 == NULL) {
    return 1;
  }
#ifdef HAVE_STRCASECMP
  return strcasecmp (str0, str1);
#else
  for (; my_tolower(*str0) == my_tolower(*str1); str0++, str1++) {
    if (*str0 == '\0') {
      return 0;
    }
  }

  return ((int) (unsigned char) my_tolower(*str0))
    - ((int) (unsigned char) my_tolower(*str1));
#endif
}

/***************************************************************
  Compare strings like strncmp(), but ignoring case.
  ie, only compares first n chars.
***************************************************************/
int mystrncasecmp(const char *str0, const char *str1, size_t n)
{
  if (str0 == NULL) {
    return -1;
  }
  if (str1 == NULL) {
    return 1;
  }
#ifdef HAVE_STRNCASECMP
  return strncasecmp (str0, str1, n);
#else
  size_t i;
  
  for (i = 0; i < n && my_tolower(*str0) == my_tolower(*str1);
       i++, str0++, str1++) {
    if (*str0 == '\0') {
      return 0;
    }
  }

  if (i == n)
    return 0;
  else
    return ((int) (unsigned char) my_tolower(*str0))
      - ((int) (unsigned char) my_tolower(*str1));
#endif
}

/***************************************************************
  Count length of string without possible surrounding quotes.
***************************************************************/
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

/***************************************************************
  Compare strings like strncasecmp() but ignoring surrounding
  quotes in either string.
***************************************************************/
int mystrncasequotecmp(const char *str0, const char *str1, size_t n)
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
    if (my_tolower(*str0) != my_tolower(*str1)) {
      return ((int) (unsigned char) my_tolower(*str0))
             - ((int) (unsigned char) my_tolower(*str1));
    }
  }

  /* All characters compared and all matched */
  return 0;
}

/***************************************************************
  Return the needle in the haystack (or NULL).
  Naive implementation.
***************************************************************/
char *mystrcasestr(const char *haystack, const char *needle)
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
    if (0 == mystrncasecmp(p, needle, needles)) {
      return (char *)p;
    }
  }
  return NULL;
#endif
}

/***************************************************************
  Returns last error code.
***************************************************************/
fc_errno fc_get_errno(void)
{
#ifdef WIN32_NATIVE
  return GetLastError();
#else
  return errno;
#endif
}

/***************************************************************
  Return a string which describes a given error (errno-style.)
  The string is converted as necessary from the local_encoding
  to internal_encoding, for inclusion in translations.  May be
  subsequently converted back to local_encoding for display.

  Note that this is not the reentrant form.
***************************************************************/
const char *fc_strerror(fc_errno err)
{
#ifdef WIN32_NATIVE
  static char buf[256];

  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		     NULL, err, 0, buf, sizeof(buf), NULL)) {
    my_snprintf(buf, sizeof(buf),
		_("error %ld (failed FormatMessage)"), err);
  }
  return buf;
#else
#ifdef HAVE_STRERROR
  static char buf[256];

  return local_to_internal_string_buffer(strerror(err),
                                         buf, sizeof(buf));
#else
  static char buf[64];

  my_snprintf(buf, sizeof(buf),
	      _("error %d (compiled without strerror)"), err);
  return buf;
#endif
#endif
}


/***************************************************************
  Suspend execution for the specified number of microseconds.
***************************************************************/
void myusleep(unsigned long usec)
{
#ifdef HAVE_USLEEP
  usleep(usec);
#else
#ifdef HAVE_SNOOZE		/* BeOS */
  snooze(usec);
#else
#ifdef GENERATING_MAC
  EventRecord the_event;	/* dummy - always be a null event */
  usec /= 16666;		/* microseconds to 1/60th seconds */
  if (usec < 1) usec = 1;
  /* suposed to give other application processor time for the mac */
  WaitNextEvent(0, &the_event, usec, 0L);
#else
#ifdef WIN32_NATIVE
  Sleep(usec / 1000);
#else
  struct timeval tv;
  tv.tv_sec=0;
  tv.tv_usec=usec;
  /* FIXME: an interrupt can cause an EINTR return here.  In that case we
   * need to have another select call. */
  my_select(0, NULL, NULL, NULL, &tv);
#endif
#endif
#endif
#endif
}


/**********************************************************************
 mystrlcpy() and mystrlcat() provide (non-standard) functions
 strlcpy() and strlcat(), with semantics following OpenBSD (and
 maybe others).  They are intended as more user-friendly
 versions of strncpy and strncat, in particular easier to
 use safely and correctly, and ensuring nul-terminated results
 while being able to detect truncation.

 n is the full size of the destination buffer, including
 space for trailing nul, and including the pre-existing
 string for mystrlcat().  Thus can eg use sizeof(buffer),
 or exact size malloc-ed.

 Result is always nul-terminated, whether or not truncation occurs,
 and the return value is the strlen the destination would have had
 without truncation.  I.e., a return value >= input n indicates
 truncation occurred.

 Will assume that if configure found strlcpy/strlcat they are ok.
 For replacement implementations, will keep it simple rather
 than try for super-efficiency.

 Not sure about the asserts below, but they are easier than
 trying to ensure correct behaviour on strange inputs.
 In particular note that n==0 is prohibited (eg, since there
 must at least be room for a nul); could consider other options.
***********************************************************************/
size_t mystrlcpy(char *dest, const char *src, size_t n)
{
  assert(dest != NULL);
  assert(src != NULL);
  assert(n>0);
#ifdef HAVE_STRLCPY
  return strlcpy(dest, src, n);
#else
  {
    size_t len = strlen(src);
    size_t num_to_copy = (len >= n) ? n-1 : len;
    if (num_to_copy>0)
      memcpy(dest, src, num_to_copy);
    dest[num_to_copy] = '\0';
    return len;
  }
#endif
}

/**********************************************************************
 ...
***********************************************************************/
size_t mystrlcat(char *dest, const char *src, size_t n)
{
  assert(dest != NULL);
  assert(src != NULL);
  assert(n>0);
#ifdef HAVE_STRLCAT
  return strlcat(dest, src, n);
#else
  {
    size_t num_to_copy, len_dest, len_src;
    
    len_dest = strlen(dest);
    assert(len_dest<n);
    /* Otherwise have bad choice of leaving dest not nul-terminated
     * within the specified length n (which should be assumable as
     * a post-condition of mystrlcat), or modifying dest before end
     * of existing string (which breaks strcat semantics).
     */
       
    dest += len_dest;
    n -= len_dest;
    
    len_src = strlen(src);
    num_to_copy = (len_src >= n) ? n-1 : len_src;
    if (num_to_copy>0)
      memcpy(dest, src, num_to_copy);
    dest[num_to_copy] = '\0';
    return len_dest + len_src;
  }
#endif
}

/**********************************************************************
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

***********************************************************************/

/* "64k should be big enough for anyone" ;-) */
#define VSNP_BUF_SIZE (64*1024)
int my_vsnprintf(char *str, size_t n, const char *format, va_list ap)
{
  int r;

  /* This may be overzealous, but I suspect any triggering of these to
   * be bugs.  */

  assert(str != NULL);
  assert(n>0);
  assert(format != NULL);

#ifdef HAVE_WORKING_VSNPRINTF
  r = vsnprintf(str, n, format, ap);
  str[n - 1] = 0;

  /* Convert C99 return value to C89.  */
  if (r >= n)
    return -1;

  return r;
#else
  {
    /* Don't use fc_malloc() or freelog() here, since they may call
       my_vsnprintf() if it fails.  */
 
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
    r = vsnprintf(buf, n, format, ap);
#else
    r = vsprintf(buf, format, ap);
#endif
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
#endif  
}

int my_snprintf(char *str, size_t n, const char *format, ...)
{
  int ret;
  va_list ap;

  assert(format != NULL);
  
  va_start(ap, format);
  ret = my_vsnprintf(str, n, format, ap);
  va_end(ap);
  return ret;
}

/**********************************************************************
  Call gethostname() if supported, else just returns -1.
***********************************************************************/
int my_gethostname(char *buf, size_t len)
{
#ifdef HAVE_GETHOSTNAME
  return gethostname(buf, len);
#else
  return -1;
#endif
}

#ifdef SOCKET_ZERO_ISNT_STDIN
/**********************************************************************
  Support for console I/O in case SOCKET_ZERO_ISNT_STDIN.
***********************************************************************/

#define MYBUFSIZE 100
static char mybuf[MYBUFSIZE+1];

/**********************************************************************/

#ifdef WIN32_NATIVE
static HANDLE mythread = INVALID_HANDLE_VALUE;

static unsigned int WINAPI thread_proc(LPVOID data)
{
  if (fgets(mybuf, MYBUFSIZE, stdin)) {
    char *s;
    if ((s = strchr(mybuf, '\n')))
      *s = '\0';
  }
  return 0;
}
#endif

/**********************************************************************
  Initialize console I/O in case SOCKET_ZERO_ISNT_STDIN.
***********************************************************************/
void my_init_console(void)
{
#ifdef WIN32_NATIVE
  unsigned int threadid;

  if (mythread != INVALID_HANDLE_VALUE)
    return;

  mybuf[0] = '\0';
  mythread = (HANDLE)_beginthreadex(NULL, 0, thread_proc, NULL, 0, &threadid);
#else
  static int initialized = 0;
  if (!initialized) {
    initialized = 1;
#ifdef HAVE_FILENO
    my_nonblock(fileno(stdin));
#endif
  }
#endif
}

/**********************************************************************
  Read a line from console I/O in case SOCKET_ZERO_ISNT_STDIN.

  This returns a pointer to a statically allocated buffer.
  Subsequent calls to my_read_console() or my_init_console() will
  overwrite it.
***********************************************************************/
char *my_read_console(void)
{
#ifdef WIN32_NATIVE
  if (WaitForSingleObject(mythread, 0) == WAIT_OBJECT_0) {
    CloseHandle(mythread);
    mythread = INVALID_HANDLE_VALUE;
    return mybuf;
  }
  return NULL;
#else
  if (!feof(stdin)) {    /* input from server operator */
    static char *bufptr = mybuf;
    /* fetch chars until \n, or run out of space in buffer */
    /* blocks if my_nonblock() in my_init_console() failed */
    while ((*bufptr = fgetc(stdin)) != EOF) {
      if (*bufptr == '\n') *bufptr = '\0';
      if (*bufptr == '\0') {
	bufptr = mybuf;
	return mybuf;
      }
      if ((bufptr - mybuf) <= MYBUFSIZE) bufptr++; /* prevent overrun */
    }
  }
  return NULL;
#endif
}

#endif

/**********************************************************************
  Returns TRUE iff the file is a regular file or a link to a regular
  file or write_access is TRUE and the file doesn't exists yet.
***********************************************************************/
bool is_reg_file_for_access(const char *name, bool write_access)
{
  struct stat tmp;

  if (stat(name, &tmp) == 0) {
    return S_ISREG(tmp.st_mode);
  } else {
    return write_access && errno == ENOENT;
  }
}

/**********************************************************************
  Character function wrappers

  Some OS's (win32, Solaris) require a non-negative value as input
  for these functions.
***********************************************************************/

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
bool my_isalnum(char c)
{
  return isalnum((int)((unsigned char)c)) != 0;
}

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
bool my_isalpha(char c)
{
  return isalpha((int)((unsigned char)c)) != 0;
}

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
bool my_isdigit(char c)
{
  return isdigit((int)((unsigned char)c)) != 0;
}

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
bool my_isprint(char c)
{
  return isprint((int)((unsigned char)c)) != 0;
}

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
bool my_isspace(char c)
{
  return isspace((int)((unsigned char)c)) != 0;
}

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
bool my_isupper(char c)
{
  return isupper((int)((unsigned char)c)) != 0;
}

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
char my_toupper(char c)
{
  return (char) toupper((int)((unsigned char)c));
}

/**********************************************************************
  Wrapper function to work around broken libc implementations.
***********************************************************************/
char my_tolower(char c)
{
  return (char) tolower((int)((unsigned char)c));
}
