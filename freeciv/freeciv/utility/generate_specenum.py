#!/usr/bin/env python

#
# Freeciv - Copyright (C) 2009
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#

# The maximum number of enumerators.
max_enum_values=150

# Here are push all defined macros.
macros=[]

import os, sys

def make_header(file):
    file.write('''
 /**************************************************************************
 *                       THIS FILE WAS GENERATED                           *
 * Script: utility/generate_specenum.py                                    *
 *                       DO NOT CHANGE THIS FILE                           *
 **************************************************************************/

/********************************************************************** 
 Freeciv - Copyright (C) 2009
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
''')

def make_documentation(file):
    file.write('''
/*
 * Include this file to define tools to manage enumerators.  First of all,
 * before including this file, you *MUST* define the following macros:
 * - SPECENUM_NAME: is the name of the enumeration (e.g. 'foo' for defining
 * 'enum foo').
 * - SPECENUM_VALUE%d: define like this all values of your enumeration type
 * (e.g. '#define SPECENUM_VALUE0 FOO_FIRST').
 *
 * The following macros *CAN* be defined:
 * - SPECENUM_INVALID: specifies a value that your 'foo_invalid()' function
 * will return.  Note it cannot be a declared value with SPECENUM_VALUE%d.
 * - SPECENUM_BITWISE: defines if the enumeration should be like
 * [1, 2, 4, 8, etc...] instead of the default of [0, 1, 2, 3, etc...].
 * - SPECENUM_ZERO: can be defined only if SPECENUM_BITWISE was also defined.
 * It defines a 0 value.  Note that if you don't declare this value, 0 passed
 * to the 'foo_is_valid()' function will return 0.
 * - SPECENUM_COUNT: a name for the maximum enumeration number plus 1. For
 * enums where every element from 0 to the maximum is defined, this is the
 * number of elements in the enum. This value is suitable to size an array
 * indexed by the enum. It can not be used in combination with
 * SPECENUM_BITWISE. SPECENUM_is_valid() will return the invalid element
 * for it.
 *
 * SPECENUM_VALUE%dNAME, SPECENUM_ZERONAME, SPECENUM_COUNTNAME: Can be used
 * to bind a string to the particular enumerator to be returned by
 * SPECENUM_name(), etc. If not defined, the default name for 'FOO_FIRST'
 * is '"FOO_FIRST"'. A name can be qualified. The qualification will only
 * be used for its translation. The returned name will be unqualified. To
 * mark a name as translatable use N_().
 *
 * SPECENUM_NAMEOVERRIDE: call callback function foo_name_cb(enum foo),
 * defined by specnum user, to get name of the enum value. If the function
 * returns NULL, compiled in names are used.
 *
 * SPECENUM_BITVECTOR: specifies the name of a bit vector for the enum
 * values. It can not be used in combination with SPECENUM_BITWISE.
 *
 * Assuming SPECENUM_NAME were 'foo', including this file would provide
 * the definition for the enumeration type 'enum foo', and prototypes for
 * the following functions:
 *   bool foo_is_bitwise(void);
 *   enum foo foo_min(void);
 *   enum foo foo_max(void);
 *   enum foo foo_invalid(void);
 *   bool foo_is_valid(enum foo);
 *
 *   enum foo foo_begin(void);
 *   enum foo foo_end(void);
 *   enum foo foo_next(enum foo);
 *
 *   const char *foo_name(enum foo);
 *   const char *foo_translated_name(enum foo);
 *   enum foo foo_by_name(const char *name,
 *                        int (*strcmp_func)(const char *, const char *));
 *
 * Example:
 *   #define SPECENUM_NAME test
 *   #define SPECENUM_BITWISE
 *   #define SPECENUM_VALUE0 TEST0
 *   #define SPECENUM_VALUE1 TEST1
 *   #define SPECENUM_VALUE3 TEST3
 *   #include "specenum_gen.h"
 *
 *  {
 *    static const char *strings[] = {
 *      "TEST1", "test3", "fghdf", NULL
 *    };
 *    enum test e;
 *    int i;
 *
 *    log_verbose("enum test [%d; %d]%s",
 *                test_min(), test_max(), test_bitwise ? " bitwise" : "");
 *
 *    for (e = test_begin(); e != test_end(); e = test_next(e)) {
 *      log_verbose("Value %d is %s", e, test_name(e));
 *    }
 *
 *    for (i = 0; strings[i]; i++) {
 *      e = test_by_name(strings[i], mystrcasecmp);
 *      if (test_is_valid(e)) {
 *        log_verbose("Value is %d for %s", e, strings[i]);
 *      } else {
 *        log_verbose("%s is not a valid name", strings[i]);
 *      }
 *    }
 *  }
 *
 * Will output:
 *   enum test [1, 8] bitwise
 *   Value 1 is TEST0
 *   Value 2 is TEST1
 *   Value 8 is TEST3
 *   Value is 2 for TEST1
 *   Value is 8 for test3
 *   fghdf is not a valid name
 */
''')

def make_macros(file):
    file.write('''
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Utility */
#include "fcintl.h"     /* translation */
#include "log.h"        /* fc_assert. */
#include "support.h"    /* bool type. */

#ifndef SPECENUM_NAME
#error Must define a SPECENUM_NAME to use this header
#endif

#define SPECENUM_PASTE_(x, y) x ## y
#define SPECENUM_PASTE(x, y) SPECENUM_PASTE_(x, y)

#define SPECENUM_STRING_(x) #x
#define SPECENUM_STRING(x) SPECENUM_STRING_(x)

#define SPECENUM_FOO(suffix) SPECENUM_PASTE(SPECENUM_NAME, suffix)

#ifndef SPECENUM_INVALID
#define SPECENUM_INVALID ((enum SPECENUM_NAME) -1)
#endif

#ifdef SPECENUM_BITWISE
#ifdef SPECENUM_COUNT
#error Cannot define SPECENUM_COUNT when SPECENUM_BITWISE is defined.
#endif
#define SPECENUM_VALUE(value) (1 << value)
#else /* SPECENUM_BITWISE */
#ifdef SPECENUM_ZERO
#error Cannot define SPECENUM_ZERO when SPECENUM_BITWISE is not defined.
#endif
#define SPECENUM_VALUE(value) (value)
#endif /* SPECENUM_BITWISE */

#ifdef SPECENUM_BITVECTOR
#include "bitvector.h"
#ifdef SPECENUM_BITWISE
#error SPECENUM_BITWISE and SPECENUM_BITVECTOR cannot both be defined.
#endif /* SPECENUM_BITWISE */
#endif /* SPECENUM_BITVECTOR */

#undef SPECENUM_MIN_VALUE
#undef SPECENUM_MAX_VALUE
''')
    macros.append("SPECENUM_NAME")
    macros.append("SPECENUM_PASTE_")
    macros.append("SPECENUM_PASTE")
    macros.append("SPECENUM_STRING_")
    macros.append("SPECENUM_STRING")
    macros.append("SPECENUM_FOO")
    macros.append("SPECENUM_INVALID")
    macros.append("SPECENUM_BITWISE")
    macros.append("SPECENUM_VALUE")
    macros.append("SPECENUM_ZERO")
    macros.append("SPECENUM_MIN_VALUE")
    macros.append("SPECENUM_MAX_VALUE")
    macros.append("SPECENUM_SIZE")
    macros.append("SPECENUM_NAMEOVERRIDE")
    macros.append("SPECENUM_BITVECTOR")

def make_enum(file):
    file.write('''
/* Enumeration definition. */
enum SPECENUM_NAME {
#ifdef SPECENUM_ZERO
  SPECENUM_ZERO = 0,
#endif
''')

    for i in range(max_enum_values):
        file.write('''
#ifdef SPECENUM_VALUE%d
  SPECENUM_VALUE%d = SPECENUM_VALUE(%d),
#  ifndef SPECENUM_MIN_VALUE
#    define SPECENUM_MIN_VALUE SPECENUM_VALUE%d
#  endif
#  ifdef SPECENUM_MAX_VALUE
#    undef SPECENUM_MAX_VALUE
#  endif
#  define SPECENUM_MAX_VALUE SPECENUM_VALUE%d
#  ifdef SPECENUM_SIZE
#    undef SPECENUM_SIZE
#  endif
#  define SPECENUM_SIZE (%d + 1)
#endif /* SPECENUM_VALUE%d */
'''%(i,i,i,i,i,i,i))

    file.write('''
#ifdef SPECENUM_COUNT
  SPECENUM_COUNT = (SPECENUM_MAX_VALUE + 1),
#endif /* SPECENUM_COUNT */
};
''')

    macros.append("SPECENUM_COUNT")
    for i in range(max_enum_values):
        macros.append("SPECENUM_VALUE%d"%i)

def make_is_bitwise(file):
    file.write('''
/**************************************************************************
  Returns TRUE if this enumeration is in bitwise mode.
**************************************************************************/
fc__attribute((const))
static inline bool SPECENUM_FOO(_is_bitwise)(void)
{
#ifdef SPECENUM_BITWISE
  return TRUE;
#else
  return FALSE;
#endif
}
''')

def make_min(file):
    file.write('''
/**************************************************************************
  Returns the value of the minimal enumerator.
**************************************************************************/
fc__attribute((const))
static inline enum SPECENUM_NAME SPECENUM_FOO(_min)(void)
{
  return SPECENUM_MIN_VALUE;
}
''')

def make_max(file):
    file.write('''
/**************************************************************************
  Returns the value of the maximal enumerator.
**************************************************************************/
fc__attribute((const))
static inline enum SPECENUM_NAME SPECENUM_FOO(_max)(void)
{
  return SPECENUM_MAX_VALUE;
}
''')

def make_is_valid(file):
    file.write('''
/**************************************************************************
  Returns TRUE if this enumerator was defined.
**************************************************************************/
fc__attribute((const))
static inline bool SPECENUM_FOO(_is_valid)(enum SPECENUM_NAME enumerator)
{
#ifdef SPECENUM_BITWISE
  static const unsigned long valid = (
    0''')

    for i in range(max_enum_values):
        file.write('''
#  ifdef SPECENUM_VALUE%d
    | SPECENUM_VALUE%d
#  endif'''%(i,i))

    file.write('''
  );

  FC_STATIC_ASSERT(sizeof(valid) * 8 >= SPECENUM_SIZE,
                   valid_sizeof_check);

#  ifdef SPECENUM_ZERO
  if (enumerator == SPECENUM_ZERO) {
    return TRUE;
  }
#  endif
  return (enumerator & valid) == enumerator;
#else
  static const bool valid[] = {''')

    for i in range(max_enum_values):
        file.write('''
#  if %d < SPECENUM_SIZE
#    ifdef SPECENUM_VALUE%d
       TRUE,
#    else
       FALSE,
#    endif
#  endif'''%(i,i))

    file.write('''
  };

  FC_STATIC_ASSERT(ARRAY_SIZE(valid) == SPECENUM_SIZE,
                   valid_array_size_check);

  return (enumerator >= 0
          && enumerator < ARRAY_SIZE(valid)
          && valid[enumerator]);
#endif /* SPECENUM_BITWISE */
}
''')

def make_invalid(file):
    file.write('''
/**************************************************************************
  Returns an invalid enumerator value.
**************************************************************************/
fc__attribute((const))
static inline enum SPECENUM_NAME SPECENUM_FOO(_invalid)(void)
{
  fc_assert(!SPECENUM_FOO(_is_valid(SPECENUM_INVALID)));
  return SPECENUM_INVALID;
}
''')

def make_begin(file):
    file.write('''
/**************************************************************************
  Beginning of the iteration of the enumerators.
**************************************************************************/
fc__attribute((const))
static inline enum SPECENUM_NAME SPECENUM_FOO(_begin)(void)
{
  return SPECENUM_FOO(_min)();
}
''')

def make_end(file):
    file.write('''
/**************************************************************************
  End of the iteration of the enumerators.
**************************************************************************/
fc__attribute((const))
static inline enum SPECENUM_NAME SPECENUM_FOO(_end)(void)
{
  return SPECENUM_FOO(_invalid)();
}
''')

def make_next(file):
    file.write('''
/**************************************************************************
  Find the next valid enumerator value.
**************************************************************************/
fc__attribute((const))
static inline enum SPECENUM_NAME SPECENUM_FOO(_next)(enum SPECENUM_NAME e)
{
  do {
#ifdef SPECENUM_BITWISE
    e = (enum SPECENUM_NAME)(e << 1);
#else
    e = (enum SPECENUM_NAME)(e + 1);
#endif

    if (e > SPECENUM_FOO(_max)()) {
      /* End of the iteration. */
      return SPECENUM_FOO(_invalid)();
    }
  } while (!SPECENUM_FOO(_is_valid)(e));

  return e;
}
''')

def make_name(file):
    file.write('''
#ifdef SPECENUM_NAMEOVERRIDE
const char *SPECENUM_FOO(_name_cb)(enum SPECENUM_NAME value);
#endif /* SPECENUM_NAMEOVERRIDE */

/**************************************************************************
  Returns the name of the enumerator.
**************************************************************************/
#ifndef SPECENUM_NAMEOVERRIDE
fc__attribute((const))
#endif
static inline const char *SPECENUM_FOO(_name)(enum SPECENUM_NAME enumerator)
{
#ifdef SPECENUM_COUNT
  static const char *names[SPECENUM_SIZE + 1];
#else
  static const char *names[SPECENUM_SIZE];
#endif
  static bool initialized = FALSE;

#ifdef SPECENUM_NAMEOVERRIDE
  {
    const char *name = SPECENUM_FOO(_name_cb)(enumerator);

    if (name != NULL) {
      return Qn_(name);
    }
  }
#endif /* SPECENUM_NAMEOVERRIDE */

  if (!initialized) {''')

    for i in range(max_enum_values):
        file.write('''
#if %d < SPECENUM_SIZE
#  ifndef SPECENUM_VALUE%d
     names[%d] = NULL;
#  elif defined(SPECENUM_VALUE%dNAME)
     names[%d] = Qn_(SPECENUM_VALUE%dNAME);
#  else
     names[%d] = SPECENUM_STRING(SPECENUM_VALUE%d);
#  endif
#endif'''%(i,i,i,i,i,i,i,i))
        macros.append("SPECENUM_VALUE%dNAME"%i)

    file.write('''
#ifdef SPECENUM_COUNT
#  ifdef SPECENUM_COUNTNAME
  names[SPECENUM_COUNT] = Qn_(SPECENUM_COUNTNAME);
#  else
  names[SPECENUM_COUNT] = SPECENUM_STRING(SPECENUM_COUNT);
#  endif
#endif
    initialized = TRUE;
  }

#ifdef SPECENUM_BITWISE
#  ifdef SPECENUM_ZERO
  if (enumerator == SPECENUM_ZERO) {
#    ifdef SPECENUM_ZERONAME
    return Qn_(SPECENUM_ZERONAME);
#    else
    return SPECENUM_STRING(SPECENUM_ZERO);
#    endif
  }
#  endif
  {
    size_t i;

    for (i = 0; i < ARRAY_SIZE(names); i++) {
      if (1 << i == enumerator) {
        return names[i];
      }
    }
  }
#else
  if (enumerator >= 0 && enumerator < ARRAY_SIZE(names)) {
    return names[enumerator];
  }
#endif /* SPECENUM_BITWISE */
  return NULL;
}
''')
    macros.append("SPECENUM_COUNTNAME")
    macros.append("SPECENUM_ZERONAME")

def make_by_name(file):
    file.write('''
/**************************************************************************
  Returns the enumerator for the name or *_invalid() if not found.
**************************************************************************/
static inline enum SPECENUM_NAME SPECENUM_FOO(_by_name)
    (const char *name, int (*strcmp_func)(const char *, const char *))
{
  enum SPECENUM_NAME e;
  const char *enum_name;

  for (e = SPECENUM_FOO(_begin)(); e != SPECENUM_FOO(_end)();
       e = SPECENUM_FOO(_next)(e)) {
    if ((enum_name = SPECENUM_FOO(_name)(e))
        && 0 == strcmp_func(name, enum_name)) {
      return e;
    }
  }

  return SPECENUM_FOO(_invalid)();
}
''')

def make_translated_name(file):
    file.write('''
/**************************************************************************
  Returns the translated name of the enumerator.
**************************************************************************/
#ifndef SPECENUM_NAMEOVERRIDE
fc__attribute((const))
#endif
static inline const char *
SPECENUM_FOO(_translated_name)(enum SPECENUM_NAME enumerator)
{
#ifdef SPECENUM_COUNT
  static const char *names[SPECENUM_SIZE + 1];
#else
  static const char *names[SPECENUM_SIZE];
#endif
  static bool initialized = FALSE;

#ifdef SPECENUM_NAMEOVERRIDE
  {
    const char *name = SPECENUM_FOO(_name_cb)(enumerator);

    if (name != NULL) {
      return Q_(name);
    }
  }
#endif /* SPECENUM_NAMEOVERRIDE */

  if (!initialized) {''')

    for i in range(max_enum_values):
        file.write('''
#if %d < SPECENUM_SIZE
#  ifndef SPECENUM_VALUE%d
     names[%d] = NULL;
#  elif defined(SPECENUM_VALUE%dNAME)
     names[%d] = Q_(SPECENUM_VALUE%dNAME);
#  else
     names[%d] = SPECENUM_STRING(SPECENUM_VALUE%d);
#  endif
#endif'''%(i,i,i,i,i,i,i,i))
        macros.append("SPECENUM_VALUE%dNAME"%i)

    file.write('''
#ifdef SPECENUM_COUNT
#  ifdef SPECENUM_COUNTNAME
  names[SPECENUM_COUNT] = Q_(SPECENUM_COUNTNAME);
#  else
  names[SPECENUM_COUNT] = SPECENUM_STRING(SPECENUM_COUNT);
#  endif
#endif
    initialized = TRUE;
  }

#ifdef SPECENUM_BITWISE
#  ifdef SPECENUM_ZERO
  if (enumerator == SPECENUM_ZERO) {
#    ifdef SPECENUM_ZERONAME
    return Q_(SPECENUM_ZERONAME);
#    else
    return SPECENUM_STRING(SPECENUM_ZERO);
#    endif
  }
#  endif
  {
    size_t i;

    for (i = 0; i < ARRAY_SIZE(names); i++) {
      if (1 << i == enumerator) {
        return names[i];
      }
    }
  }
#else
  if (enumerator >= 0 && enumerator < ARRAY_SIZE(names)) {
    return names[enumerator];
  }
#endif /* SPECENUM_BITWISE */
  return NULL;
}
''')

def make_bitvector(file):
    file.write('''
#ifdef SPECENUM_BITVECTOR
BV_DEFINE(SPECENUM_BITVECTOR, (SPECENUM_MAX_VALUE + 1));
#endif /* SPECENUM_BITVECTOR */
''')

def make_undef(file):
    for macro in macros:
        file.write('''
#undef %s'''%macro)

    file.write('''
''')

# Main function.
def main():
    target_name=sys.argv[1]

    output=open(target_name,"w")

    make_header(output)
    make_documentation(output)
    make_macros(output)
    make_enum(output)
    make_is_bitwise(output)
    make_min(output)
    make_max(output)
    make_is_valid(output)
    make_invalid(output)
    make_begin(output)
    make_end(output)
    make_next(output)
    make_name(output)
    make_by_name(output)
    make_translated_name(output)
    make_bitvector(output)
    make_undef(output)

    output.write('''
#ifdef __cplusplus
}
#endif /* __cplusplus */
''')

    output.close()

main()
