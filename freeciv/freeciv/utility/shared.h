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
#ifndef FC__SHARED_H
#define FC__SHARED_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <freeciv_config.h>

#include <stdlib.h>		/* size_t */
#include <string.h>		/* memset */
#include <time.h>		/* time_t */

/* utility */
#include "log.h"
#include "support.h" /* bool, fc__attribute */

/* Changing these will break network compatability! */
#define MAX_LEN_ADDR     256	/* see also MAXHOSTNAMELEN and RFC 1123 2.1 */
#define MAX_LEN_PATH    4095

/* Use FC_INFINITY to denote that a certain event will never occur or
   another unreachable condition. */
#define FC_INFINITY    	(1000 * 1000 * 1000)

#ifndef FREECIV_TESTMATIC
/* Initialize something for the sole purpose of silencing false compiler warning
 * about variable possibly used uninitialized. */
#define BAD_HEURISTIC_INIT(_ini_val_) = _ini_val_
#else  /* FREECIV_TESTMATIC */
#define BAD_HEURISTIC_INIT(_ini_val_)
#endif /* FREECIV_TESTMATIC */

enum fc_tristate { TRI_NO, TRI_YES, TRI_MAYBE };
#define BOOL_TO_TRISTATE(tri) ((tri) ? TRI_YES : TRI_NO)

enum fc_tristate fc_tristate_and(enum fc_tristate one,
                                 enum fc_tristate two);

#ifndef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif
#define CLIP(lower,current,upper) \
    ((current)<(lower)?(lower):(current)>(upper)?(upper):(current))

#ifndef ABS
#define ABS(x) (((x) >= 0) ? (x) : -(x))
#endif

/* Note: Solaris already has a WRAP macro that is completely different. */
#define FC_WRAP(value, range)                                               \
    ((value) < 0                                                            \
     ? ((value) % (range) != 0 ? (value) % (range) + (range) : 0)           \
     : ((value) >= (range) ? (value) % (range) : (value)))

#define BOOL_VAL(x) ((x) != 0)
#define XOR(p, q) (BOOL_VAL(p) != BOOL_VAL(q))
#define EQ(p, q) (BOOL_VAL(p) == BOOL_VAL(q))

/*
 * DIVIDE() divides and rounds down, rather than just divides and
 * rounds toward 0.  It is assumed that the divisor is positive.
 */
#define DIVIDE(n, d) \
    ( (n) / (d) - (( (n) < 0 && (n) % (d) < 0 ) ? 1 : 0) )

#define MAX_UINT32 0xFFFFFFFF
#define MAX_UINT16 0xFFFF
#define MAX_UINT8 0xFF

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define ADD_TO_POINTER(p, n) ((void *)((char *)(p)+(n)))

#define FC_MEMBER(type, member) (((type *) NULL)->member)
#define FC_MEMBER_OFFSETOF(type, member) ((size_t) &FC_MEMBER(type, member))
#define FC_MEMBER_SIZEOF(type, member) sizeof(FC_MEMBER(type, member))
#define FC_MEMBER_ARRAY_SIZE(type, member) \
  ARRAY_SIZE(FC_MEMBER(type, member))

#define FC_INT_TO_PTR(i) ((void *) (intptr_t) (i))
#define FC_PTR_TO_INT(p) ((int) (intptr_t) (p))
#define FC_UINT_TO_PTR(u) ((void *) (intptr_t) (u))
#define FC_PTR_TO_UINT(p) ((unsigned int) (intptr_t) (p))
#define FC_SIZE_TO_PTR(s) ((void *) (intptr_t) (s))
#define FC_PTR_TO_SIZE(p) ((size_t) (intptr_t) (p))

/****************************************************************************
  Used to initialize an array 'a' of size 'size' with value 'val' in each
  element. Note that the value is evaluated for each element.
****************************************************************************/
#define INITIALIZE_ARRAY(array, size, value)				    \
  {									    \
    int _ini_index;							    \
    									    \
    for (_ini_index = 0; _ini_index < (size); _ini_index++) {		    \
      (array)[_ini_index] = (value);					    \
    }									    \
  }

#ifndef PATH_SEPARATOR
#if defined(FREECIV_MSWINDOWS) || defined(_WIN32) || defined(__WIN32__) || defined(__EMX__) || defined(__DJGPP__)
  /* Win32, OS/2, DOS */
# define PATH_SEPARATOR ";"
#else
  /* Unix */
# define PATH_SEPARATOR ":"
#endif
#endif /* PATH_SEPARATOR */

#if defined(FREECIV_MSWINDOWS) || defined(_WIN32) || defined(__WIN32__) || defined(__EMX__) || defined(__DJGPP__)
  /* Win32, OS/2, DOS */
# define DIR_SEPARATOR "\\"
# define DIR_SEPARATOR_CHAR '\\'
#else
  /* Unix */
# define DIR_SEPARATOR_IS_DEFAULT
# define DIR_SEPARATOR "/"
# define DIR_SEPARATOR_CHAR '/'
#endif

#define PARENT_DIR_OPERATOR ".."

const char *big_int_to_text(unsigned int mantissa, unsigned int exponent);
const char *int_to_text(unsigned int number);

bool is_ascii_name(const char *name);
bool is_base64url(const char *s);
bool is_safe_filename(const char *name);
void randomize_base64url_string(char *s, size_t n);

int compare_strings(const void *first, const void *second);
int compare_strings_ptrs(const void *first, const void *second);
int compare_strings_strvec(const char *const *first,
                           const char *const *second);

char *skip_leading_spaces(char *s);
void remove_leading_spaces(char *s);
void remove_trailing_spaces(char *s);
void remove_leading_trailing_spaces(char *s);

bool check_strlen(const char *str, size_t len, const char *errmsg);
size_t loud_strlcpy(char *buffer, const char *str, size_t len,
		   const char *errmsg);
/* Convenience macro. */
#define sz_loud_strlcpy(buffer, str, errmsg) \
    loud_strlcpy(buffer, str, sizeof(buffer), errmsg)

char *end_of_strn(char *str, int *nleft);

bool str_to_int(const char *str, int *pint);
bool str_to_float(const char *str, float *pfloat);

/**************************************************************************
...
**************************************************************************/
struct fileinfo {
  char *name;           /* descriptive file name string */
  char *fullname;       /* full absolute filename */
  time_t mtime;         /* last modification time  */
};

#define SPECLIST_TAG fileinfo
#define SPECLIST_TYPE struct fileinfo
#include "speclist.h"
#define fileinfo_list_iterate(list, pnode) \
  TYPED_LIST_ITERATE(struct fileinfo, list, pnode)
#define fileinfo_list_iterate_end LIST_ITERATE_END

char *user_home_dir(void);
void free_user_home_dir(void);
char *user_username(char *buf, size_t bufsz);
char *freeciv_storage_dir(void);
void free_freeciv_storage_dir(void);

const struct strvec *get_data_dirs(void);
const struct strvec *get_save_dirs(void);
const struct strvec *get_scenario_dirs(void);

void free_data_dir_names(void);

struct strvec *fileinfolist(const struct strvec *dirs, const char *suffix);
struct fileinfo_list *fileinfolist_infix(const struct strvec *dirs,
                                         const char *infix, bool nodups);
const char *fileinfoname(const struct strvec *dirs, const char *filename);
void free_fileinfo_data(void);

void init_nls(void);
void free_nls(void);
char *setup_langname(void);
void switch_lang(const char *lang);

void dont_run_as_root(const char *argv0, const char *fallback);

/*** matching prefixes: ***/

enum m_pre_result {
  M_PRE_EXACT,		/* matches with exact length */
  M_PRE_ONLY,		/* only matching prefix */
  M_PRE_AMBIGUOUS,	/* first of multiple matching prefixes */
  M_PRE_EMPTY,		/* prefix is empty string (no match) */
  M_PRE_LONG,		/* prefix is too long (no match) */
  M_PRE_FAIL,		/* no match at all */
  M_PRE_LAST		/* flag value */
};

const char *m_pre_description(enum m_pre_result result);

/* function type to access a name from an index: */
typedef const char *(*m_pre_accessor_fn_t)(int);

/* function type to compare prefix: */
typedef int (*m_pre_strncmp_fn_t)(const char *, const char *, size_t n);

/* function type to calculate effective string length: */
typedef size_t (m_strlen_fn_t)(const char *str);

enum m_pre_result match_prefix(m_pre_accessor_fn_t accessor_fn,
			       size_t n_names,
			       size_t max_len_name,
			       m_pre_strncmp_fn_t cmp_fn,
                               m_strlen_fn_t len_fn,
			       const char *prefix,
			       int *ind_result);
enum m_pre_result match_prefix_full(m_pre_accessor_fn_t accessor_fn,
                                    size_t n_names,
                                    size_t max_len_name,
                                    m_pre_strncmp_fn_t cmp_fn,
                                    m_strlen_fn_t len_fn,
                                    const char *prefix,
                                    int *ind_result,
                                    int *matches,
                                    int max_matches,
                                    int *pnum_matches);

char *get_multicast_group(bool ipv6_preferred);
void free_multicast_group(void);
void interpret_tilde(char* buf, size_t buf_size, const char* filename);
char *interpret_tilde_alloc(const char* filename);
char *skip_to_basename(char *filepath);

bool make_dir(const char *pathname);
bool path_is_absolute(const char *filename);

char scanin(const char **buf, char *delimiters, char *dest, int size);

void array_shuffle(int *array, int n);

void format_time_duration(time_t t, char *buf, int maxlen);

bool wildcard_fit_string(const char *pattern, const char *test);

/* Custom format strings. */
struct cf_sequence;

int fc_snprintcf(char *buf, size_t buf_len, const char *format, ...)
     fc__attribute((nonnull (1, 3)));   /* Not a printf format. */
int fc_vsnprintcf(char *buf, size_t buf_len, const char *format,
                  const struct cf_sequence *sequences, size_t sequences_num)
     fc__attribute((nonnull (1, 3, 4)));

/* Tools for fc_snprintcf(). */
static inline struct cf_sequence cf_bool_seq(char letter, bool value);
static inline struct cf_sequence cf_trans_bool_seq(char letter, bool value);
static inline struct cf_sequence cf_char_seq(char letter, char value);
static inline void cf_int_seq(char letter, int value, struct cf_sequence *out);
static inline struct cf_sequence cf_hexa_seq(char letter, int value);
static inline struct cf_sequence cf_float_seq(char letter, float value);
static inline struct cf_sequence cf_ptr_seq(char letter, const void *value);
static inline struct cf_sequence cf_str_seq(char letter, const char *value);
static inline struct cf_sequence cf_end(void);

enum cf_type {
  CF_BOOLEAN,
  CF_TRANS_BOOLEAN,
  CF_CHARACTER,
  CF_INTEGER,
  CF_HEXA,
  CF_FLOAT,
  CF_POINTER,
  CF_STRING,

  CF_LAST = -1
};

struct cf_sequence {
  enum cf_type type;
  char letter;
  union {
    bool bool_value;
    char char_value;
    int int_value;
    float float_value;
    const void *ptr_value;
    const char *str_value;
  };
};

/****************************************************************************
  Build an argument for fc_snprintcf() of boolean type.
****************************************************************************/
static inline struct cf_sequence cf_bool_seq(char letter, bool value)
{
  struct cf_sequence sequence;

  sequence.type = CF_BOOLEAN;
  sequence.letter = letter;
  sequence.bool_value = value;

  return sequence;
}

/****************************************************************************
  Build an argument for fc_snprintcf() of boolean type (result will be
  translated).
****************************************************************************/
static inline struct cf_sequence cf_trans_bool_seq(char letter, bool value)
{
  struct cf_sequence sequence;

  sequence.type = CF_TRANS_BOOLEAN;
  sequence.letter = letter;
  sequence.bool_value = value;

  return sequence;
}

/****************************************************************************
  Build an argument for fc_snprintcf() of character type (%c).
****************************************************************************/
static inline struct cf_sequence cf_char_seq(char letter, char value)
{
  struct cf_sequence sequence;

  sequence.type = CF_CHARACTER;
  sequence.letter = letter;
  sequence.char_value = value;

  return sequence;
}

/****************************************************************************
  Build an argument for fc_snprintcf() of integer type (%d).
****************************************************************************/
static inline void cf_int_seq(char letter, int value, struct cf_sequence *out)
{
  out->type = CF_INTEGER;
  out->letter = letter;
  out->int_value = value;
}

/****************************************************************************
  Build an argument for fc_snprintcf() of hexadecimal type (%x).
****************************************************************************/
static inline struct cf_sequence cf_hexa_seq(char letter, int value)
{
  struct cf_sequence sequence;

  sequence.type = CF_HEXA;
  sequence.letter = letter;
  sequence.int_value = value;

  return sequence;
}

/****************************************************************************
  Build an argument for fc_snprintcf() of float type (%f).
****************************************************************************/
static inline struct cf_sequence cf_float_seq(char letter, float value)
{
  struct cf_sequence sequence;

  sequence.type = CF_FLOAT;
  sequence.letter = letter;
  sequence.float_value = value;

  return sequence;
}

/****************************************************************************
  Build an argument for fc_snprintcf() of pointer type (%p).
****************************************************************************/
static inline struct cf_sequence cf_ptr_seq(char letter, const void *value)
{
  struct cf_sequence sequence;

  sequence.type = CF_POINTER;
  sequence.letter = letter;
  sequence.ptr_value = value;

  return sequence;
}

/****************************************************************************
  Build an argument for fc_snprintcf() of string type (%s).
****************************************************************************/
static inline struct cf_sequence cf_str_seq(char letter, const char *value)
{
  struct cf_sequence sequence;

  sequence.type = CF_STRING;
  sequence.letter = letter;
  sequence.str_value = value;

  return sequence;
}

/****************************************************************************
  Must finish the list of the arguments of fc_snprintcf().
****************************************************************************/
static inline struct cf_sequence cf_end(void)
{
  struct cf_sequence sequence;

  sequence.type = CF_LAST;

  return sequence;
}

bool formats_match(const char *format1, const char *format2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__SHARED_H */
