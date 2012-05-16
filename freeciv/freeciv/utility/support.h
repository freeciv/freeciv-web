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

#ifndef FC__SUPPORT_H
#define FC__SUPPORT_H

/********************************************************************** 
  Replacements for functions which are not available on all platforms.
  Where the functions are available natively, these are just wrappers.
  See also mem.h, netintf.h, rand.h, and see support.c for more comments.
***********************************************************************/

#include <stdarg.h>
#include <stdlib.h>		/* size_t */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE true
#define FALSE false

#if __BEOS__
#include <posix/be_prim.h>
#define __bool_true_false_are_defined 1
#else
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else /* Implement <stdbool.h> ourselves */
#undef bool
#undef true
#undef false
#undef __bool_true_false_are_defined
#define bool fc_bool
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
typedef unsigned int fc_bool;
#endif /* ! HAVE_STDBOOL_H */
#endif /* ! __BEOS__ */

/* Want to use GCC's __attribute__ keyword to check variadic
 * parameters to printf-like functions, without upsetting other
 * compilers: put any required defines magic here.
 * If other compilers have something equivalent, could also
 * work that out here.   Should this use configure stuff somehow?
 * --dwp
 */
#if defined(__GNUC__)
#define fc__attribute(x)  __attribute__(x)
#else
#define fc__attribute(x)
#endif

#ifdef WIN32_NATIVE
typedef long int fc_errno;
#else
typedef int fc_errno;
#endif

int mystrcasecmp(const char *str0, const char *str1);
int mystrncasecmp(const char *str0, const char *str1, size_t n);
int mystrncasequotecmp(const char *str0, const char *str1, size_t n);

size_t effectivestrlenquote(const char *str);

char *mystrcasestr(const char *haystack, const char *needle);

fc_errno fc_get_errno(void);
const char *fc_strerror(fc_errno err);
void myusleep(unsigned long usec);

size_t mystrlcpy(char *dest, const char *src, size_t n);
size_t mystrlcat(char *dest, const char *src, size_t n);

/* convenience macros for use when dest is a char ARRAY: */
#define sz_strlcpy(dest,src) ((void)mystrlcpy((dest),(src),sizeof(dest)))
#define sz_strlcat(dest,src) ((void)mystrlcat((dest),(src),sizeof(dest)))

int my_snprintf(char *str, size_t n, const char *format, ...)
     fc__attribute((__format__ (__printf__, 3, 4)));

int my_vsnprintf(char *str, size_t n, const char *format, va_list ap );

int my_gethostname(char *buf, size_t len);

#ifdef SOCKET_ZERO_ISNT_STDIN
/* Support for console I/O in case SOCKET_ZERO_ISNT_STDIN. */
void my_init_console(void);
char *my_read_console(void);
#endif

bool is_reg_file_for_access(const char *name, bool write_access);

bool my_isalnum(char c);
bool my_isalpha(char c);
bool my_isdigit(char c);
bool my_isprint(char c);
bool my_isspace(char c);
bool my_isupper(char c);
char my_toupper(char c);
char my_tolower(char c);

#endif  /* FC__SUPPORT_H */
