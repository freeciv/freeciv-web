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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
/* Under Mac OS X sys/types.h must be included before dirent.h */
#include <sys/types.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32_NATIVE
#include <windows.h>
#include <lmcons.h>	/* UNLEN */
#include <shlobj.h>
#endif

#include "astring.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"

#include "shared.h"

#ifndef PATH_SEPARATOR
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  /* Win32, OS/2, DOS */
# define PATH_SEPARATOR ";"
#else
  /* Unix */
# define PATH_SEPARATOR ":"
#endif
#endif

#define PARENT_DIR_OPERATOR ".."

/* If no default data path is defined use the default default one */
#ifndef DEFAULT_DATA_PATH
#define DEFAULT_DATA_PATH "." PATH_SEPARATOR "data" PATH_SEPARATOR \
                          "~/.freeciv/" DATASUBDIR
#endif

/* environment */
#ifndef FREECIV_PATH
#define FREECIV_PATH "FREECIV_PATH"
#endif

/* Both of these are stored in the local encoding.  The grouping_sep must
 * be converted to the internal encoding when it's used. */
static char *grouping = NULL;
static char *grouping_sep = NULL;

static const char base64url[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";


/***************************************************************
  Take a string containing multiple lines and create a copy where
  each line is padded to the length of the longest line and centered.
  We do not cope with tabs etc.  Note that we're assuming that the
  last line does _not_ end with a newline.  The caller should
  free() the result.

  FIXME: This is only used in the Xaw client, and so probably does
  not belong in common.
***************************************************************/
char *create_centered_string(const char *s)
{
  /* Points to the part of the source that we're looking at. */
  const char *cp;

  /* Points to the beginning of the line in the source that we're
   * looking at. */
  const char *cp0;

  /* Points to the result. */
  char *r;

  /* Points to the part of the result that we're filling in right
     now. */
  char *rn;

  int i;

  int maxlen = 0;
  int curlen = 0;
  int nlines = 1;

  for(cp=s; *cp != '\0'; cp++) {
    if(*cp!='\n')
      curlen++;
    else {
      if(maxlen<curlen)
	maxlen=curlen;
      curlen=0;
      nlines++;
    }
  }
  if(maxlen<curlen)
    maxlen=curlen;
  
  r=rn=fc_malloc(nlines*(maxlen+1));
  
  curlen=0;
  for(cp0=cp=s; *cp != '\0'; cp++) {
    if(*cp!='\n')
      curlen++;
    else {
      for(i=0; i<(maxlen-curlen)/2; i++)
	*rn++=' ';
      memcpy(rn, cp0, curlen);
      rn+=curlen;
      *rn++='\n';
      curlen=0;
      cp0=cp+1;
    }
  }
  for(i=0; i<(maxlen-curlen)/2; i++)
    *rn++=' ';
  strcpy(rn, cp0);

  return r;
}

/**************************************************************************
  return a char * to the parameter of the option or NULL.
  *i can be increased to get next string in the array argv[].
  It is an error for the option to exist but be an empty string.
  This doesn't use freelog() because it is used before logging is set up.

  The argv strings are assumed to be in the local encoding; the returned
  string is in the internal encoding.
**************************************************************************/
char *get_option_malloc(const char *option_name,
			char **argv, int *i, int argc)
{
  int len = strlen(option_name);

  if (strcmp(option_name, argv[*i]) == 0 ||
      (strncmp(option_name, argv[*i], len) == 0 && argv[*i][len] == '=') ||
      strncmp(option_name + 1, argv[*i], 2) == 0) {
    char *opt = argv[*i] + (argv[*i][1] != '-' ? 0 : len);

    if (*opt == '=') {
      opt++;
    } else {
      if (*i < argc - 1) {
	(*i)++;
	opt = argv[*i];
	if (strlen(opt)==0) {
	  fc_fprintf(stderr, _("Empty argument for \"%s\".\n"), option_name);
	  exit(EXIT_FAILURE);
	}
      }	else {
	fc_fprintf(stderr, _("Missing argument for \"%s\".\n"), option_name);
	exit(EXIT_FAILURE);
      }
    }

    return local_to_internal_string_malloc(opt);
  }

  return NULL;
}

/***************************************************************
...
***************************************************************/
bool is_option(const char *option_name,char *option)
{
  return (strcmp(option_name, option) == 0 ||
	  strncmp(option_name + 1, option, 2) == 0);
}

/***************************************************************
  Like strcspn but also handles quotes, i.e. *reject chars are 
  ignored if they are inside single or double quotes.
***************************************************************/
static size_t my_strcspn(const char *s, const char *reject)
{
  bool in_single_quotes = FALSE, in_double_quotes = FALSE;
  size_t i, len = strlen(s);

  for (i = 0; i < len; i++) {
    if (s[i] == '"' && !in_single_quotes) {
      in_double_quotes = !in_double_quotes;
    } else if (s[i] == '\'' && !in_double_quotes) {
      in_single_quotes = !in_single_quotes;
    }

    if (in_single_quotes || in_double_quotes) {
      continue;
    }

    if (strchr(reject, s[i])) {
      break;
    }
  }

  return i;
}

/***************************************************************
 Splits the string into tokens. The individual tokens are
 returned. The delimiterset can freely be chosen.

 i.e. "34 abc 54 87" with a delimiterset of " " will yield 
      tokens={"34", "abc", "54", "87"}
 
 Part of the input string can be quoted (single or double) to embedded
 delimiter into tokens. For example,
   command 'a name' hard "1,2,3,4,5" 99
   create 'Mack "The Knife"'
 will yield 5 and 2 tokens respectively using the delimiterset " ,".

 Tokens which aren't used aren't modified (and memory is not
 allocated). If the string would yield more tokens only the first
 num_tokens are extracted.

 The user has the responsiblity to free the memory allocated by
 **tokens using free_tokens().
***************************************************************/
int get_tokens(const char *str, char **tokens, size_t num_tokens,
	       const char *delimiterset)
{
  int token = 0;

  assert(str != NULL);

  for(;;) {
    size_t len, padlength = 0;

    /* skip leading delimiters */
    str += strspn(str, delimiterset);

    if (*str == '\0') {
      break;
    }

    len = my_strcspn(str, delimiterset);

    if (token >= num_tokens) {
      break;
    }

    /* strip start/end quotes if they exist */
    if (len >= 2) {
      if ((str[0] == '"' && str[len - 1] == '"')
	  || (str[0] == '\'' && str[len - 1] == '\'')) {
	len -= 2;
	padlength = 1;		/* to set the string past the end quote */
	str++;
      }
    }
  
    tokens[token] = fc_malloc(len + 1);
    (void) mystrlcpy(tokens[token], str, len + 1);	/* adds the '\0' */

    token++;

    str += len + padlength;
  }

  return token;
}

/***************************************************************
  Frees a set of tokens created by get_tokens().
***************************************************************/
void free_tokens(char **tokens, size_t ntokens)
{
  size_t i;
  for (i = 0; i < ntokens; i++) {
    if (tokens[i]) {
      free(tokens[i]);
    }
  }
}

/***************************************************************
  Returns a statically allocated string containing a nicely-formatted
  version of the given number according to the user's locale.  (Only
  works for numbers >= zero.)  The number is given in scientific notation
  as mantissa * 10^exponent.
***************************************************************/
const char *big_int_to_text(unsigned int mantissa, unsigned int exponent)
{
  static char buf[64]; /* Note that we'll be filling this in right to left. */
  char *grp = grouping;
  char *ptr;
  unsigned int cnt = 0;
  char sep[64];
  size_t seplen;

  /* We have to convert the encoding here (rather than when the locale
   * is initialized) because it can't be done before the charsets are
   * initialized. */
  local_to_internal_string_buffer(grouping_sep, sep, sizeof(sep));
  seplen = strlen(sep);

#if 0 /* Not needed while the values are unsigned. */
  assert(mantissa >= 0);
  assert(exponent >= 0);
#endif

  if (mantissa == 0) {
    return "0";
  }

  /* We fill the string in backwards, starting from the right.  So the first
   * thing we do is terminate it. */
  ptr = &buf[sizeof(buf)];
  *(--ptr) = '\0';

  while (mantissa != 0 && exponent >= 0) {
    int dig;

    if (ptr <= buf + seplen) {
      /* Avoid a buffer overflow. */
      assert(ptr > buf + seplen);
      return ptr;
    }

    /* Add on another character. */
    if (exponent > 0) {
      dig = 0;
      exponent--;
    } else {
      dig = mantissa % 10;
      mantissa /= 10;
    }
    *(--ptr) = '0' + dig;

    cnt++;
    if (mantissa != 0 && cnt == *grp) {
      /* Reached count of digits in group: insert separator and reset count. */
      cnt = 0;
      if (*grp == CHAR_MAX) {
	/* This test is unlikely to be necessary since we would need at
	   least 421-bit ints to break the 127 digit barrier, but why not. */
	break;
      }
      ptr -= seplen;
      assert(ptr >= buf);
      memcpy(ptr, sep, seplen);
      if (*(grp + 1) != 0) {
	/* Zero means to repeat the present group-size indefinitely. */
        grp++;
      }
    }
  }

  return ptr;
}


/****************************************************************************
  Return a prettily formatted string containing the given number.
****************************************************************************/
const char *int_to_text(unsigned int number)
{
  return big_int_to_text(number, 0);
}

/****************************************************************************
  Check whether or not the given char is a valid ascii character.  The
  character can be in any charset so long as it is a superset of ascii.
****************************************************************************/
static bool is_ascii(char ch)
{
  /* this works with both signed and unsigned char's. */
  return ch >= ' ' && ch <= '~';
}

/****************************************************************************
  Check if the name is safe security-wise.  This is intended to be used to
  make sure an untrusted filename is safe to be used. 
****************************************************************************/
bool is_safe_filename(const char *name)
{
  int i = 0;

  /* must not be NULL or empty */
  if (!name || *name == '\0') {
    return FALSE; 
  }

  for (; '\0' != name[i]; i++) {
    if ('.' != name[i] && NULL == strchr(base64url, name[i])) {
      return FALSE;
    }
  }

  /* we don't allow the filename to ascend directories */
  if (strstr(name, PARENT_DIR_OPERATOR)) {
    return FALSE;
  }

  /* Otherwise, it is okay... */
  return TRUE;
}

/***************************************************************
  This is used in sundry places to make sure that names of cities,
  players etc. do not contain yucky characters of various sorts.
  Returns TRUE iff the name is acceptable.
  FIXME:  Not internationalised.
***************************************************************/
bool is_ascii_name(const char *name)
{
  const char illegal_chars[] = {'|', '%', '"', ',', '*', '<', '>', '\0'};
  int i, j;

  /* must not be NULL or empty */
  if (!name || *name == '\0') {
    return FALSE; 
  }

  /* must begin and end with some non-space character */
  if ((*name == ' ') || (*(strchr(name, '\0') - 1) == ' ')) {
    return FALSE;
  }

  /* must be composed entirely of printable ascii characters,
   * and no illegal characters which can break ranking scripts. */
  for (i = 0; name[i]; i++) {
    if (!is_ascii(name[i])) {
      return FALSE;
    }
    for (j = 0; illegal_chars[j]; j++) {
      if (name[i] == illegal_chars[j]) {
	return FALSE;
      }
    }
  }

  /* otherwise, it's okay... */
  return TRUE;
}

/*************************************************************************
  Check for valid base64url.
*************************************************************************/
bool is_base64url(const char *s)
{
  size_t i = 0;

  /* must not be NULL or empty */
  if (NULL == s || '\0' == *s) {
    return FALSE; 
  }

  for (; '\0' != s[i]; i++) {
    if (NULL == strchr(base64url, s[i])) {
      return FALSE;
    }
  }
  return TRUE;
}

/*************************************************************************
  generate a random string meeting criteria such as is_ascii_name(),
  is_base64url(), and is_safe_filename().
*************************************************************************/
void randomize_base64url_string(char *s, size_t n)
{
  size_t i = 0;

  /* must not be NULL or too short */
  if (NULL == s || 1 > n) {
    return; 
  }

  for (; i < (n - 1); i++) {
    s[i] = base64url[myrand(sizeof(base64url) - 1)];
  }
  s[i] = '\0';
}

/**************************************************************************
  Compares two strings, in the collating order of the current locale,
  given pointers to the two strings (i.e., given "char *"s).
  Case-sensitive.  Designed to be called from qsort().
**************************************************************************/
int compare_strings(const void *first, const void *second)
{
#if defined(ENABLE_NLS) && defined(HAVE_STRCOLL)
  return strcoll((const char *)first, (const char *)second);
#else
  return strcmp((const char *)first, (const char *)second);
#endif
}

/**************************************************************************
  Compares two strings, in the collating order of the current locale,
  given pointers to the two string pointers (i.e., given "char **"s).
  Case-sensitive.  Designed to be called from qsort().
**************************************************************************/
int compare_strings_ptrs(const void *first, const void *second)
{
#if defined(ENABLE_NLS) && defined(HAVE_STRCOLL)
  return strcoll(*((const char **)first), *((const char **)second));
#else
  return strcmp(*((const char **)first), *((const char **)second));
#endif
}

/***************************************************************************
  Returns 's' incremented to first non-space character.
***************************************************************************/
char *skip_leading_spaces(char *s)
{
  assert(s!=NULL);
  while(*s != '\0' && my_isspace(*s)) {
    s++;
  }
  return s;
}

/***************************************************************************
  Removes leading spaces in string pointed to by 's'.
  Note 's' must point to writeable memory!
***************************************************************************/
static void remove_leading_spaces(char *s)
{
  char *t;
  
  assert(s!=NULL);
  t = skip_leading_spaces(s);
  if (t != s) {
    while (*t != '\0') {
      *s++ = *t++;
    }
    *s = '\0';
  }
}

/***************************************************************************
  Terminates string pointed to by 's' to remove traling spaces;
  Note 's' must point to writeable memory!
***************************************************************************/
static void remove_trailing_spaces(char *s)
{
  char *t;
  size_t len;
  
  assert(s!=NULL);
  len = strlen(s);
  if (len > 0) {
    t = s + len -1;
    while(my_isspace(*t)) {
      *t = '\0';
      if (t == s) {
	break;
      }
      t--;
    }
  }
}

/***************************************************************************
  Removes leading and trailing spaces in string pointed to by 's'.
  Note 's' must point to writeable memory!
***************************************************************************/
void remove_leading_trailing_spaces(char *s)
{
  remove_leading_spaces(s);
  remove_trailing_spaces(s);
}

/***************************************************************************
  As remove_trailing_spaces(), for specified char.
***************************************************************************/
static void remove_trailing_char(char *s, char trailing)
{
  char *t;
  
  assert(s!=NULL);
  t = s + strlen(s) -1;
  while(t>=s && (*t) == trailing) {
    *t = '\0';
    t--;
  }
}

/***************************************************************************
  Change spaces in s into newlines, so as to keep lines length len
  or shorter.  That is, modifies s.
  Returns number of lines in modified s.
***************************************************************************/
int wordwrap_string(char *s, int len)
{
  int num_lines = 0;
  int slen = strlen(s);
  
  /* At top of this loop, s points to the rest of string,
   * either at start or after inserted newline: */
 top:
  if (s && *s != '\0' && slen > len) {
    char *c;

    num_lines++;
    
    /* check if there is already a newline: */
    for(c=s; c<s+len; c++) {
      if (*c == '\n') {
	slen -= c+1 - s;
	s = c+1;
	goto top;
      }
    }
    
    /* find space and break: */
    for(c=s+len; c>s; c--) {
      if (my_isspace(*c)) {
	*c = '\n';
	slen -= c+1 - s;
	s = c+1;
	goto top;
      }
    }

    /* couldn't find a good break; settle for a bad one... */
    for (c = s + len + 1; *c != '\0'; c++) {
      if (my_isspace(*c)) {
	*c = '\n';
	slen -= c+1 - s;
	s = c+1;
	goto top;
      }
    }
  }
  return num_lines;
}

/***************************************************************************
  Returns pointer to '\0' at end of string 'str', and decrements
  *nleft by the length of 'str'.  This is intended to be useful to
  allow strcat-ing without traversing the whole string each time,
  while still keeping track of the buffer length.
  Eg:
     char buf[128];
     int n = sizeof(buf);
     char *p = buf;

     my_snprintf(p, n, "foo%p", p);
     p = end_of_strn(p, &n);
     mystrlcpy(p, "yyy", n);
***************************************************************************/
char *end_of_strn(char *str, int *nleft)
{
  int len = strlen(str);
  *nleft -= len;
  assert((*nleft)>0);		/* space for the terminating nul */
  return str + len;
}

/********************************************************************** 
  Check the length of the given string.  If the string is too long,
  log errmsg, which should be a string in printf-format taking up to
  two arguments: the string and the length.
**********************************************************************/ 
bool check_strlen(const char *str, size_t len, const char *errmsg)
{
  if (strlen(str) >= len) {
    freelog(LOG_ERROR, errmsg, str, len);
    assert(0);
    return TRUE;
  }
  return FALSE;
}

/********************************************************************** 
  Call check_strlen() on str and then strlcpy() it into buffer.
**********************************************************************/
size_t loud_strlcpy(char *buffer, const char *str, size_t len,
		   const char *errmsg)
{
  (void) check_strlen(str, len, errmsg);
  return mystrlcpy(buffer, str, len);
}

/********************************************************************** 
 cat_snprintf is like a combination of my_snprintf and mystrlcat;
 it does snprintf to the end of an existing string.
 
 Like mystrlcat, n is the total length available for str, including
 existing contents and trailing nul.  If there is no extra room
 available in str, does not change the string. 

 Also like mystrlcat, returns the final length that str would have
 had without truncation.  I.e., if return is >= n, truncation occurred.
**********************************************************************/ 
int cat_snprintf(char *str, size_t n, const char *format, ...)
{
  size_t len;
  int ret;
  va_list ap;

  assert(format != NULL);
  assert(str != NULL);
  assert(n>0);
  
  len = strlen(str);
  assert(len < n);
  
  va_start(ap, format);
  ret = my_vsnprintf(str+len, n-len, format, ap);
  va_end(ap);
  return (int) (ret + len);
}

/***************************************************************************
  Exit because of a fatal error after printing a message about it.  This
  should only be called for code errors - user errors (like not being able
  to find a tileset) should just exit rather than dumping core.
***************************************************************************/
void real_die(const char *file, int line, const char *format, ...)
{
  va_list ap;

  freelog(LOG_FATAL, "Detected fatal error in %s line %d:", file, line);
  va_start(ap, format);
  vreal_freelog(LOG_FATAL, format, ap);
  va_end(ap);

  assert(FALSE);

  exit(EXIT_FAILURE);
}

/***************************************************************************
  Returns string which gives users home dir, as specified by $HOME.
  Gets value once, and then caches result.
  If $HOME is not set, give a log message and returns NULL.
  Note the caller should not mess with the returned string.
***************************************************************************/
char *user_home_dir(void)
{
#ifdef AMIGA
  return "PROGDIR:";
#else
  static bool init = FALSE;
  static char *home_dir = NULL;
  
  if (!init) {
    char *env = getenv("HOME");
    if (env) {
      home_dir = mystrdup(env);	        /* never free()d */
      freelog(LOG_VERBOSE, "HOME is %s", home_dir);
    } else {

#ifdef WIN32_NATIVE

      /* some documentation at: 
       * http://justcheckingonall.wordpress.com/2008/05/16/find-shell-folders-win32/
       * http://archives.seul.org/or/cvs/Oct-2004/msg00082.html */

      LPITEMIDLIST pidl;
      LPMALLOC pMalloc;
      
      if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl))) {
        
        home_dir = fc_malloc(PATH_MAX);
        
        if (!SUCCEEDED(SHGetPathFromIDList(pidl, home_dir))) {
          free(home_dir);
          home_dir = NULL;
          freelog(LOG_ERROR,
            "Could not find home directory (SHGetPathFromIDList() failed)");
        }
        
        SHGetMalloc(&pMalloc);
        if (pMalloc) {
          pMalloc->lpVtbl->Free(pMalloc, pidl);
          pMalloc->lpVtbl->Release(pMalloc);
        }

      } else {
        freelog(LOG_ERROR,
          "Could not find home directory (SHGetSpecialFolderLocation() failed)");
      }
#else
      freelog(LOG_ERROR, "Could not find home directory (HOME is not set)");
      home_dir = NULL;
#endif
    }
    init = TRUE;
  }
  return home_dir;
#endif
}

/***************************************************************************
  Returns string which gives user's username, as specified by $USER or
  as given in password file for this user's uid, or a made up name if
  we can't get either of the above. 
  Gets value once, and then caches result.
  Note the caller should not mess with returned string.
***************************************************************************/
char *user_username(char *buf, size_t bufsz)
{
  /* This function uses a number of different methods to try to find a
   * username.  This username then has to be truncated to bufsz
   * characters (including terminator) and checked for sanity.  Note that
   * truncating a sane name can leave you with an insane name under some
   * charsets. */

  /* If the environment variable $USER is present and sane, use it. */
  {
    char *env = getenv("USER");

    if (env) {
      mystrlcpy(buf, env, bufsz);
      if (is_ascii_name(buf)) {
	freelog(LOG_VERBOSE, "USER username is %s", buf);
	return buf;
      }
    }
  }

#ifdef HAVE_GETPWUID
  /* Otherwise if getpwuid() is available we can use it to find the true
   * username. */
  {
    struct passwd *pwent = getpwuid(getuid());

    if (pwent) {
      mystrlcpy(buf, pwent->pw_name, bufsz);
      if (is_ascii_name(buf)) {
	freelog(LOG_VERBOSE, "getpwuid username is %s", buf);
	return buf;
      }
    }
  }
#endif

#ifdef WIN32_NATIVE
  /* On win32 the GetUserName function will give us the login name. */
  {
    char name[UNLEN + 1];
    DWORD length = sizeof(name);

    if (GetUserName(name, &length)) {
      mystrlcpy(buf, name, bufsz);
      if (is_ascii_name(buf)) {
	freelog(LOG_VERBOSE, "GetUserName username is %s", buf);
	return buf;
      }
    }
  }
#endif

#ifdef ALWAYS_ROOT
  mystrlcpy(buf, "name", bufsz);
#else
  my_snprintf(buf, bufsz, "name%d", (int)getuid());
#endif
  freelog(LOG_VERBOSE, "fake username is %s", buf);
  assert(is_ascii_name(buf));
  return buf;
}

/***************************************************************************
  Returns a list of data directory paths, in the order in which they should
  be searched.  These paths are specified internally or may be set as the
  environment variable $FREECIV_PATH (a separated list of directories,
  where the separator itself is specified internally, platform-dependent).
  '~' at the start of a component (provided followed by '/' or '\0') is
  expanded as $HOME.

  The returned value is a static NULL-terminated list of strings.

  num_dirs, if not NULL, will be set to the number of entries in the list.
***************************************************************************/
const char **get_data_dirs(int *num_dirs)
{
  const char *path;
  char *path2, *tok;
  static int num = 0;
  static const char **dirs = NULL;

  /* The first time this function is called it will search and
   * allocate the directory listing.  Subsequently we will already
   * know the list and can just return it. */
  if (dirs) {
    if (num_dirs) {
      *num_dirs = num;
    }
    return dirs;
  }

  path = getenv(FREECIV_PATH);
  if (!path) {
    path = DEFAULT_DATA_PATH;
  } else if (*path == '\0') {
    freelog(LOG_ERROR,
            /* TRANS: <FREECIV_PATH> configuration error */
            _("\"%s\" is set but empty; using default \"%s\" instead."),
            FREECIV_PATH, DEFAULT_DATA_PATH);
    path = DEFAULT_DATA_PATH;
  }
  assert(path != NULL);
  
  path2 = mystrdup(path);	/* something we can strtok */
    
  tok = strtok(path2, PATH_SEPARATOR);
  do {
    int i;			/* strlen(tok), or -1 as flag */

    tok = skip_leading_spaces(tok);
    remove_trailing_spaces(tok);
    if (strcmp(tok, "/") != 0) {
      remove_trailing_char(tok, '/');
    }
      
    i = strlen(tok);
    if (tok[0] == '~') {
      if (i > 1 && tok[1] != '/') {
	freelog(LOG_ERROR, "For \"%s\" in data path cannot expand '~'"
		" except as '~/'; ignoring", tok);
	i = 0;   /* skip this one */
      } else {
	char *home = user_home_dir();

	if (!home) {
	  freelog(LOG_VERBOSE,
		  "No HOME, skipping data path component %s", tok);
	  i = 0;
	} else {
	  int len = strlen(home) + i;	   /* +1 -1 */
	  char *tmp = fc_malloc(len);

	  my_snprintf(tmp, len, "%s%s", home, tok + 1);
	  tok = tmp;
	  i = -1;		/* flag to free tok below */
	}
      }
    }

    if (i != 0) {
      /* We could check whether the directory exists and
       * is readable etc?  Don't currently. */
      num++;
      dirs = fc_realloc(dirs, num * sizeof(char*));
      dirs[num - 1] = mystrdup(tok);
      freelog(LOG_VERBOSE, "Data path component (%d): %s", num - 1, tok);
      if (i == -1) {
	free(tok);
	tok = NULL;
      }
    }

    tok = strtok(NULL, PATH_SEPARATOR);
  } while(tok);

  /* NULL-terminate the list. */
  dirs = fc_realloc(dirs, (num + 1) * sizeof(char*));
  dirs[num] = NULL;

  free(path2);
  
  if (num_dirs) {
    *num_dirs = num;
  }
  return dirs;
}

/***************************************************************************
  Returns a NULL-terminated list of filenames in the data directories
  matching the given suffix.

  The list is allocated when the function is called; it should either
  be stored permanently or de-allocated (by free'ing each element and
  the whole list).

  The suffixes are removed from the filenames before the list is
  returned.
***************************************************************************/
char **datafilelist(const char* suffix)
{
  const char **dirs = get_data_dirs(NULL);
  char **file_list = NULL;
  int num_matches = 0;
  int list_size = 0;
  int dir_num, i, j;
  size_t suffix_len = strlen(suffix);

  assert(!strchr(suffix, '/'));

  /* First assemble a full list of names. */
  for (dir_num = 0; dirs[dir_num]; dir_num++) {
    DIR* dir;
    struct dirent *entry;

    /* Open the directory for reading. */
    dir = opendir(dirs[dir_num]);
    if (!dir) {
      if (errno == ENOENT) {
	freelog(LOG_VERBOSE, "Skipping non-existing data directory %s.",
		dirs[dir_num]);
      } else {
	/* TRANS: "...: <externally translated error string>."*/
        freelog(LOG_ERROR, _("Could not read data directory %s: %s."),
		dirs[dir_num], fc_strerror(fc_get_errno()));
      }
      continue;
    }

    /* Scan all entries in the directory. */
    while ((entry = readdir(dir))) {
      size_t len = strlen(entry->d_name);

      /* Make sure the file name matches. */
      if (len > suffix_len
	  && strcmp(suffix, entry->d_name + len - suffix_len) == 0) {
	/* Strdup the entry so we can safely write to it. */
	char *match = mystrdup(entry->d_name);

	/* Make sure the list is big enough; grow exponentially to keep
	   constant ammortized overhead. */
	if (num_matches >= list_size) {
	  list_size = list_size > 0 ? list_size * 2 : 10;
	  file_list = fc_realloc(file_list, list_size * sizeof(*file_list));
	}

	/* Clip the suffix. */
	match[len - suffix_len] = '\0';

	file_list[num_matches++] = mystrdup(match);

	free(match);
      }
    }

    closedir(dir);
  }

  /* Sort the list. */
  qsort(file_list, num_matches, sizeof(*file_list), compare_strings_ptrs);

  /* Remove duplicates (easy since it's sorted). */
  i = j = 0;
  while (j < num_matches) {
    char *this = file_list[j];

    for (j++; j < num_matches && strcmp(this, file_list[j]) == 0; j++) {
      free(file_list[j]);
    }

    file_list[i] = this;

    i++;
  }
  num_matches = i;

  /* NULL-terminate the whole thing. */
  file_list = fc_realloc(file_list, (num_matches + 1) * sizeof(*file_list));
  file_list[num_matches] = NULL;

  return file_list;
}

/***************************************************************************
  Returns a filename to access the specified file from a data
  directory by searching all data directories (as specified by
  get_data_dirs) for the file.

  If the specified 'filename' is NULL, the returned string contains
  the effective data path.  (But this should probably only be used for
  debug output.)
  
  Returns NULL if the specified filename cannot be found in any of the
  data directories.  (A file is considered "found" if it can be
  read-opened.)  The returned pointer points to static memory, so this
  function can only supply one filename at a time.
***************************************************************************/
char *datafilename(const char *filename)
{
  int num_dirs, i;
  const char **dirs = get_data_dirs(&num_dirs);
  static struct astring realfile = ASTRING_INIT;

  if (!filename) {
    size_t len = 1;		/* in case num_dirs==0 */
    size_t seplen = strlen(PATH_SEPARATOR);

    for (i = 0; i < num_dirs; i++) {
      len += strlen(dirs[i]) + MAX(1, seplen);	/* separator or '\0' */
    }
    astr_minsize(&realfile, len);
    realfile.str[0] = '\0';

    for (i = 0; i < num_dirs; i++) {
      (void) mystrlcat(realfile.str, dirs[i], len);
      if (i < num_dirs) {
	(void) mystrlcat(realfile.str, PATH_SEPARATOR, len);
      }
    }
    return realfile.str;
  }
  
  for (i = 0; i < num_dirs; i++) {
    struct stat buf;		/* see if we can open the file or directory */
    size_t len = strlen(dirs[i]) + strlen(filename) + 2;
    
    astr_minsize(&realfile, len);
    my_snprintf(realfile.str, len, "%s/%s", dirs[i], filename);
    if (stat(realfile.str, &buf) == 0) {
      return realfile.str;
    }
  }

  freelog(LOG_VERBOSE, "Could not find readable file \"%s\" in data path.",
	  filename);

  return NULL;
}

/**************************************************************************
  Compare modification times.
**************************************************************************/
static int compare_file_mtime_ptrs(const struct datafile *const *ppa,
                                   const struct datafile * const *ppb)
{
  return ((*ppa)->mtime < (*ppb)->mtime);
}

/**************************************************************************
  Compare names.
**************************************************************************/
static int compare_file_name_ptrs(const struct datafile *const *ppa,
                                  const struct datafile * const *ppb)
{
  return compare_strings((*ppa)->name, (*ppb)->name);
}

/**************************************************************************
  Search for filenames with the "infix" substring in the "subpath"
  subdirectory of the data path.
  "nodups" removes duplicate names.
  The returned list will be sorted by name first and modification time
  second. Returned "name"s will be truncated starting at the "infix"
  substring. The returned list must be freed.
**************************************************************************/
struct datafile_list *datafilelist_infix(const char *subpath,
                                         const char *infix, bool nodups)
{
  const char **dirs = get_data_dirs(NULL);
  int num_matches = 0;
  int dir_num;
  struct datafile_list *res;

  res = datafile_list_new();

  /* First assemble a full list of names. */
  for (dir_num = 0; dirs[dir_num]; dir_num++) {
    size_t len = (subpath ? strlen(subpath) : 0) + strlen(dirs[dir_num]) + 2;
    char path[len];
    DIR *dir;
    struct dirent *entry;

    if (subpath) {
      my_snprintf(path, sizeof(path), "%s/%s", dirs[dir_num], subpath);
    } else {
      sz_strlcpy(path, dirs[dir_num]);
    }

    /* Open the directory for reading. */
    dir = opendir(path);
    if (!dir) {
      continue;
    }

    /* Scan all entries in the directory. */
    while ((entry = readdir(dir))) {
      struct datafile *file;
      char *ptr;
      /* Strdup the entry so we can safely write to it. */
      char *filename = mystrdup(entry->d_name);

      /* Make sure the file name matches. */
      if ((ptr = strstr(filename, infix))) {
	struct stat buf;
	char *fullname;
	size_t len = strlen(path) + strlen(filename) + 2;

	fullname = fc_malloc(len);
	my_snprintf(fullname, len, "%s/%s", path, filename);

	if (stat(fullname, &buf) == 0) {
	  file = fc_malloc(sizeof(*file));

	  /* Clip the suffix. */
	  *ptr = '\0';

	  file->name = mystrdup(filename);
	  file->fullname = mystrdup(fullname);
	  file->mtime = buf.st_mtime;

	  datafile_list_append(res, file);
	  num_matches++;
	}

	free(fullname);
      }

      free(filename);
    }

    closedir(dir);
  }

  /* Sort the list by name. */
  datafile_list_sort(res, compare_file_name_ptrs);

  if (nodups) {
    char *name = "";

    datafile_list_iterate(res, pfile) {
      if (compare_strings(name, pfile->name) != 0) {
	name = pfile->name;
      } else {
	free(pfile->name);
	free(pfile->fullname);
	datafile_list_unlink(res, pfile);
      }
    } datafile_list_iterate_end;
  }

  /* Sort the list by last modification time. */
  datafile_list_sort(res, compare_file_mtime_ptrs);

  return res;
}

/***************************************************************************
  As datafilename(), above, except die with an appropriate log
  message if we can't find the file in the datapath.
***************************************************************************/
char *datafilename_required(const char *filename)
{
  char *dname;
  
  assert(filename!=NULL);
  dname = datafilename(filename);

  if (dname) {
    return dname;
  } else {
    freelog(LOG_ERROR,
            /* TRANS: <FREECIV_PATH> configuration error */
            _("The data path may be set via the \"%s\" environment variable."),
            FREECIV_PATH);
    freelog(LOG_ERROR,
            _("Current data path is: \"%s\""),
            datafilename(NULL));
    freelog(LOG_FATAL,
            _("The \"%s\" file is required ... aborting!"), filename);
    exit(EXIT_FAILURE);
  }
}

/***************************************************************************
  Language environmental variable (with emulation).
***************************************************************************/
char *get_langname(void)
{
  char *langname = NULL;      
        
#ifdef ENABLE_NLS

  langname = getenv("LANG");
  
#ifdef WIN32_NATIVE
  /* set LANG by hand if it is not set */
  if (!langname) {
    switch (PRIMARYLANGID(GetUserDefaultLangID())) {
      case LANG_ARABIC:
        return "ar";
      case LANG_CATALAN:
        return "ca";
      case LANG_CZECH:
        return "cs";
      case LANG_DANISH:
        return "da"; 
      case LANG_GERMAN:
        return "de";
      case LANG_GREEK:
        return "el";
      case LANG_ENGLISH:
        switch (SUBLANGID(GetUserDefaultLangID())) {
          case SUBLANG_ENGLISH_UK:
            return "en_GB";
          default:
            return "en"; 
        }
      case LANG_SPANISH:
        return "es";
      case LANG_ESTONIAN:
        return "et";
      case LANG_FARSI:
        return "fa";
      case LANG_FINNISH:
        return "fi";
      case LANG_FRENCH:
        return "fr";
      case LANG_HEBREW:
        return "he";
      case LANG_HUNGARIAN:
        return "hu";
      case LANG_ITALIAN:
        return "it";
      case LANG_JAPANESE:
        return "ja";
      case LANG_KOREAN:
        return "ko";
      case LANG_LITHUANIAN:
        return "lt";
      case LANG_DUTCH:
        return "nl";
      case LANG_NORWEGIAN:
        switch (SUBLANGID(GetUserDefaultLangID())) {
          case SUBLANG_NORWEGIAN_BOKMAL:
            return "nb";
          default:
            return "no";
        }
      case LANG_POLISH:
        return "pl";
      case LANG_PORTUGUESE:
        switch (SUBLANGID(GetUserDefaultLangID())) {
          case SUBLANG_PORTUGUESE_BRAZILIAN:
            return "pt_BR";
          default:
            return "pt";
        }
      case LANG_ROMANIAN:
        return "ro";
      case LANG_RUSSIAN:
        return "ru";
      case LANG_SWEDISH:
        return "sv";
      case LANG_TURKISH:
        return "tr";
      case LANG_UKRAINIAN:
        return "uk";
      case LANG_CHINESE:
        return "zh_CN";
    }
  }
#endif /* WIN32_NATIVE */
#endif /* ENABLE_NLS */

  return langname;
}
        
/***************************************************************************
  Setup for Native Language Support, if configured to use it.
  (Call this only once, or it may leak memory.)
***************************************************************************/
void init_nls(void)
{
  /*
   * Setup the cached locale numeric formatting information. Defaults
   * are as appropriate for the US.
   */
  grouping = mystrdup("\3");
  grouping_sep = mystrdup(",");

#ifdef ENABLE_NLS

#ifdef WIN32_NATIVE
  char *langname = get_langname();  
  if (langname) {
    static char envstr[40];

    my_snprintf(envstr, sizeof(envstr), "LANG=%s", langname);
    putenv(envstr);
  }
#endif

  (void) setlocale(LC_ALL, "");
  (void) bindtextdomain(PACKAGE, LOCALEDIR);
  (void) textdomain(PACKAGE);

  /* Don't touch the defaults when LC_NUMERIC == "C".
     This is intended to cater to the common case where:
       1) The user is from North America. ;-)
       2) The user has not set the proper environment variables.
	  (Most applications are (unfortunately) US-centric
	  by default, so why bother?)
     This would result in the "C" locale being used, with grouping ""
     and thousands_sep "", where we really want "\3" and ",". */

  if (strcmp(setlocale(LC_NUMERIC, NULL), "C") != 0) {
    struct lconv *lc = localeconv();

    if (lc->grouping[0] == '\0') {
      /* This actually indicates no grouping at all. */
      char *m = malloc(sizeof(char));
      *m = CHAR_MAX;
      grouping = m;
    } else {
      size_t len;
      for (len = 0;
	   lc->grouping[len] != '\0' && lc->grouping[len] != CHAR_MAX; len++) {
	/* nothing */
      }
      len++;
      free(grouping);
      grouping = fc_malloc(len);
      memcpy(grouping, lc->grouping, len);
    }
    free(grouping_sep);
    grouping_sep = mystrdup(lc->thousands_sep);
  }
#endif
}

/***************************************************************************
  Free memory allocated by Native Language Support
***************************************************************************/
void free_nls(void)
{
  free(grouping);
  grouping = NULL;
  free(grouping_sep);
  grouping_sep = NULL;
}

/***************************************************************************
  If we have root privileges, die with an error.
  (Eg, for security reasons.)
  Param argv0 should be argv[0] or similar; fallback is
  used instead if argv0 is NULL.
  But don't die on systems where the user is always root...
  (a general test for this would be better).
  Doesn't use freelog() because gets called before logging is setup.
***************************************************************************/
void dont_run_as_root(const char *argv0, const char *fallback)
{
#if (defined(ALWAYS_ROOT) || defined(__EMX__) || defined(__BEOS__))
  return;
#else
  if (getuid()==0 || geteuid()==0) {
    fc_fprintf(stderr,
	       _("%s: Fatal error: you're trying to run me as superuser!\n"),
	       (argv0 ? argv0 : fallback ? fallback : "freeciv"));
    fc_fprintf(stderr, _("Use a non-privileged account instead.\n"));
    exit(EXIT_FAILURE);
  }
#endif
}

/***************************************************************************
  Return a description string of the result.
  In English, form of description is suitable to substitute in, eg:
     prefix is <description>
  (N.B.: The description is always in English, but they have all been marked
   for translation.  If you want a localized version, use _() on the return.)
***************************************************************************/
const char *m_pre_description(enum m_pre_result result)
{
  static const char * const descriptions[] = {
    N_("exact match"),
    N_("only match"),
    N_("ambiguous"),
    N_("empty"),
    N_("too long"),
    N_("non-match")
  };
  assert(result >= 0 && result < ARRAY_SIZE(descriptions));
  return descriptions[result];
}

/***************************************************************************
  See match_prefix_full().
***************************************************************************/
enum m_pre_result match_prefix(m_pre_accessor_fn_t accessor_fn,
                               size_t n_names,
                               size_t max_len_name,
                               m_pre_strncmp_fn_t cmp_fn,
                               m_strlen_fn_t len_fn,
                               const char *prefix,
                               int *ind_result)
{
  return match_prefix_full(accessor_fn, n_names, max_len_name, cmp_fn,
                           len_fn, prefix, ind_result, NULL, 0, NULL);
}

/***************************************************************************
  Given n names, with maximum length max_len_name, accessed by
  accessor_fn(0) to accessor_fn(n-1), look for matching prefix
  according to given comparison function.
  Returns type of match or fail, and for return <= M_PRE_AMBIGUOUS
  sets *ind_result with matching index (or for ambiguous, first match).
  If max_len_name==0, treat as no maximum.
  If the int array 'matches' is non-NULL, up to 'max_matches' ambiguous
  matching names indices will be inserted into it. If 'pnum_matches' is
  non-NULL, it will be set to the number of indices inserted into 'matches'.
***************************************************************************/
enum m_pre_result match_prefix_full(m_pre_accessor_fn_t accessor_fn,
                                    size_t n_names,
                                    size_t max_len_name,
                                    m_pre_strncmp_fn_t cmp_fn,
                                    m_strlen_fn_t len_fn,
                                    const char *prefix,
                                    int *ind_result,
                                    int *matches,
                                    int max_matches,
                                    int *pnum_matches)
{
  int i, len, nmatches;

  if (len_fn == NULL) {
    len = strlen(prefix);
  } else {
    len = len_fn(prefix);
  }
  if (len == 0) {
    return M_PRE_EMPTY;
  }
  if (len > max_len_name && max_len_name > 0) {
    return M_PRE_LONG;
  }

  nmatches = 0;
  for(i=0; i<n_names; i++) {
    const char *name = accessor_fn(i);
    if (cmp_fn(name, prefix, len)==0) {
      if (strlen(name) == len) {
	*ind_result = i;
	return M_PRE_EXACT;
      }
      if (nmatches==0) {
	*ind_result = i;	/* first match */
      }
      if (matches != NULL && nmatches < max_matches) {
        matches[nmatches] = i;
      }
      nmatches++;
    }
  }

  if (nmatches == 1) {
    return M_PRE_ONLY;
  } else if (nmatches > 1) {
    if (pnum_matches != NULL) {
      *pnum_matches = MIN(max_matches, nmatches);
    }
    return M_PRE_AMBIGUOUS;
  } else {
    return M_PRE_FAIL;
  }
}

/***************************************************************************
 Return whether two vectors: vec1 and vec2 have common
 bits. I.e. (vec1 & vec2) != 0.

 Don't call this function directly, use BV_CHECK_MASK macro
 instead. Don't call this function with two different bitvectors.
***************************************************************************/
bool bv_check_mask(const unsigned char *vec1, const unsigned char *vec2,
		   size_t size1, size_t size2)
{
  size_t i;
  assert(size1 == size2);

  for (i = 0; i < size1; i++) {
    if ((vec1[0] & vec2[0]) != 0) {
      return TRUE;
    }
    vec1++;
    vec2++;
  }
  return FALSE;
}

bool bv_are_equal(const unsigned char *vec1, const unsigned char *vec2,
		  size_t size1, size_t size2)
{
  size_t i;
  assert(size1 == size2);

  for (i = 0; i < size1; i++) {
    if (vec1[0] != vec2[0]) {
      return FALSE;
    }
    vec1++;
    vec2++;
  }
  return TRUE;
}

/***************************************************************************
  Returns string which gives the multicast group IP address for finding
  servers on the LAN, as specified by $FREECIV_MULTICAST_GROUP.
  Gets value once, and then caches result.
***************************************************************************/
char *get_multicast_group(bool ipv6_prefered)
{
  static bool init = FALSE;
  static char *group = NULL;
  static char *default_multicast_group_ipv4 = "225.1.1.1";
#ifdef IPV6_SUPPORT
  /* TODO: Get useful group (this is node local) */
  static char *default_multicast_group_ipv6 = "FF31::8000:15B4";
#endif

  if (!init) {
    char *env = getenv("FREECIV_MULTICAST_GROUP");
    if (env) {
      group = mystrdup(env);	        
    } else {
#ifdef IPV6_SUPPORT
      if (ipv6_prefered) {
        group = mystrdup(default_multicast_group_ipv6);
      } else
#endif /* IPv6 support */
      {
        group = mystrdup(default_multicast_group_ipv4);
      }
    }
    init = TRUE;
  }
  return group;
}

/***************************************************************************
  Interpret ~/ in filename as home dir
  New path is returned in buf of size buf_size

  This may fail if the path is too long.  It is better to use
  interpret_tilde_alloc.
***************************************************************************/
void interpret_tilde(char* buf, size_t buf_size, const char* filename)
{
  if (filename[0] == '~' && filename[1] == '/') {
    my_snprintf(buf, buf_size, "%s/%s", user_home_dir(), filename + 2);
  } else if (filename[0] == '~' && filename[1] == '\0') {
    strncpy(buf, user_home_dir(), buf_size);
  } else  {
    strncpy(buf, filename, buf_size);
  }
}

/***************************************************************************
  Interpret ~/ in filename as home dir

  The new path is returned in buf, as a newly allocated buffer.  The new
  path will always be allocated and written, even if there is no ~ present.
***************************************************************************/
char *interpret_tilde_alloc(const char* filename)
{
  if (filename[0] == '~' && filename[1] == '/') {
    const char *home = user_home_dir();
    size_t sz;
    char *buf;

    filename += 2; /* Skip past "~/" */
    sz = strlen(home) + strlen(filename) + 2;
    buf = fc_malloc(sz);
    my_snprintf(buf, sz, "%s/%s", home, filename);
    return buf;
  } else if (filename[0] == '~' && filename[1] == '\0') {
    return mystrdup(user_home_dir());
  } else  {
    return mystrdup(filename);
  }
}

/**************************************************************************
  If the directory "pathname" does not exist, recursively create all
  directories until it does.
**************************************************************************/
bool make_dir(const char *pathname)
{
  char *dir;
  char *path = NULL;

  path = interpret_tilde_alloc(pathname);
  dir = path;
  do {
    dir = strchr(dir, '/');
    /* We set the current / with 0, and restore it afterwards */
    if (dir) {
      *dir = 0;
    }

#ifdef WIN32_NATIVE
    mkdir(path);
#else
    mkdir(path, 0755);
#endif
      
    if (dir) {
      *dir = '/';
      dir++;
    }
  } while (dir);

  free(path);
  return TRUE;
}

/**************************************************************************
  Returns TRUE if the filename's path is absolute.
**************************************************************************/
bool path_is_absolute(const char *filename)
{
  if (!filename) {
    return FALSE;
  }

#ifdef WIN32_NATIVE
  if (strchr(filename, ':')) {
    return TRUE;
  }
#else
  if (filename[0] == '/') {
    return TRUE;
  }
#endif  

  return FALSE;
}

/**************************************************************************
  Scan in a word or set of words from start to but not including
  any of the given delimiters. The buf pointer will point past delimiter,
  or be set to NULL if there is nothing there. Removes excess white
  space.

  This function should be safe, and dest will contain "\0" and
  *buf == NULL on failure. We always fail gently.

  Due to the way the scanning is performed, looking for a space
  will give you the first word even if it comes before multiple
  spaces.

  Returns delimiter found.

  Pass in NULL for dest and -1 for size to just skip ahead.  Note that if
  nothing is found, dest will contain the whole string, minus leading and
  trailing whitespace.  You can scan for "" to conveniently grab the
  remainder of a string.
**************************************************************************/
char scanin(char **buf, char *delimiters, char *dest, int size)
{
  char *ptr, found = '?';

  if (*buf == NULL || strlen(*buf) == 0 || size == 0) {
    if (dest) {
      dest[0] = '\0';
    }
    *buf = NULL;
    return '\0';
  }

  if (dest) {
    strncpy(dest, *buf, size-1);
    dest[size-1] = '\0';
    remove_leading_trailing_spaces(dest);
    ptr = strpbrk(dest, delimiters);
  } else {
    /* Just skip ahead. */
    ptr = strpbrk(*buf, delimiters);
  }
  if (ptr != NULL) {
    found = *ptr;
    if (dest) {
      *ptr = '\0';
    }
    if (dest) {
      remove_leading_trailing_spaces(dest);
    }
    *buf = strpbrk(*buf, delimiters);
    if (*buf != NULL) {
      (*buf)++; /* skip delimiter */
    } else {
    }
  } else {
    *buf = NULL;
  }

  return found;
}

/************************************************************************
  Randomize the elements of an array using the Fisher-Yates shuffle.

  see: http://benpfaff.org/writings/clc/shuffle.html
************************************************************************/
void array_shuffle(int *array, int n)
{
  if (n > 1 && array != NULL) {
    int i, j, t;
    for (i = 0; i < n - 1; i++) {
      j = i + myrand(n - i);
      t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}
