/*********************************************************
*                                                        *
* (c) 2011-2015 Marko Lindqvist                          *
*                                                        *
* Version contributed to Freeciv has been made available *
* under GNU Public License; either version 2, or         *
* (at your option) any later version.                    *
*                                                        *
*********************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cvercmp.h"

static char **cvercmp_ver_tokenize(const char *ver);
static int cvercmp_next_token(const char *str);
static char **cvercmp_ver_subtokenize(const char *ver);
static int cvercmp_next_subtoken(const char *str);

enum cvercmp_prever
{
  CVERCMP_PRE_DEV,
  CVERCMP_PRE_ALPHA,
  CVERCMP_PRE_BETA,
  CVERCMP_PRE_PRE,
  CVERCMP_PRE_RC,
  CVERCMP_PRE_NONE
};

struct preverstr
{
  const char *str;
  enum cvercmp_prever prever;
};

struct preverstr preverstrs[] =
{
  { "dev", CVERCMP_PRE_DEV },
  { "alpha", CVERCMP_PRE_ALPHA },
  { "beta", CVERCMP_PRE_BETA },
  { "pre", CVERCMP_PRE_PRE },
  { "candidate", CVERCMP_PRE_RC },
  { "rc", CVERCMP_PRE_RC },
  { NULL, CVERCMP_PRE_NONE }
};

static enum cvercmp_prever cvercmp_parse_prever(const char *ver);

bool cvercmp(const char *ver1, const char *ver2, enum cvercmp_type type)
{
  typedef bool (*cmpfunc)(const char *ver1, const char *ver2);

  cmpfunc cmpfuncs[] =
  {
    cvercmp_equal,
    cvercmp_nonequal,
    cvercmp_greater,
    cvercmp_lesser,
    cvercmp_min,
    cvercmp_max
  };

  return cmpfuncs[type](ver1, ver2);
}

bool cvercmp_equal(const char *ver1, const char *ver2)
{
  return cvercmp_cmp(ver1, ver2) == CVERCMP_EQUAL;
}

bool cvercmp_nonequal(const char *ver1, const char *ver2)
{
  return cvercmp_cmp(ver1, ver2) != CVERCMP_EQUAL;
}

bool cvercmp_greater(const char *ver1, const char *ver2)
{
  return cvercmp_cmp(ver1, ver2) == CVERCMP_GREATER;
}

bool cvercmp_lesser(const char *ver1, const char *ver2)
{
  return cvercmp_cmp(ver1, ver2) == CVERCMP_LESSER;
}

bool cvercmp_min(const char *ver1, const char *ver2)
{
  return cvercmp_cmp(ver1, ver2) != CVERCMP_LESSER;
}

bool cvercmp_max(const char *ver1, const char *ver2)
{
  return cvercmp_cmp(ver1, ver2) != CVERCMP_GREATER;
}

static enum cvercmp_type cvercmp_tokens(const char *token1, const char *token2)
{
  char **t1 = cvercmp_ver_subtokenize(token1);
  char **t2 = cvercmp_ver_subtokenize(token2);
  int i;
  enum cvercmp_type result = CVERCMP_EQUAL;
  bool solution = false;

  for (i = 0; (t1[i] != NULL && t2[i] != NULL) && !solution; i++) {
    int t1c0 = t1[i][0];
    int t2c0 = t2[i][0];
    bool d1 = isdigit(t1c0);
    bool d2 = isdigit(t2c0);

    if (d1 && !d2) {
      /* Numbers are always greater than alpha, so don't need to check prever */
      result = CVERCMP_GREATER;
      solution = true;
    } else if (!d1 && d2) {
      result = CVERCMP_LESSER;
      solution = true;
    } else if (d1) {
      int val1 = atoi(t1[i]);
      int val2 = atoi(t2[i]);

      if (val1 > val2) {
        result = CVERCMP_GREATER;
        solution = true;
      } else if (val1 < val2) {
        result = CVERCMP_LESSER;
        solution = true;
      }
    } else {
      int alphacmp = strcasecmp(t1[i], t2[i]);

      if (alphacmp) {
        enum cvercmp_prever pre1 = cvercmp_parse_prever(t1[i]);
        enum cvercmp_prever pre2 = cvercmp_parse_prever(t2[i]);

        if (pre1 > pre2) {
          result = CVERCMP_GREATER;
        } else if (pre1 < pre2) {
          result = CVERCMP_LESSER;
        } else if (alphacmp < 0) {
          result = CVERCMP_LESSER;
        } else if (alphacmp > 0) {
          result = CVERCMP_GREATER;
        } else {
          assert(false);
        }

        solution = true;
      }
    }
  }

  if (!solution) {
    /* Got to the end of one token.
     * Longer token is greater, unless the next subtoken is
     * a prerelease string. */
    if (t1[i] != NULL && t2[i] == NULL) {
      if (cvercmp_parse_prever(t1[i]) != CVERCMP_PRE_NONE) {
        result = CVERCMP_LESSER;
      } else {
        result = CVERCMP_GREATER;
      }
    } else if (t1[i] == NULL && t2[i] != NULL) {
      if (cvercmp_parse_prever(t2[i]) != CVERCMP_PRE_NONE) {
        result = CVERCMP_GREATER;
      } else {
        result = CVERCMP_LESSER;
      }
    } else {
      /* Both ran out at the same time. */
      result = CVERCMP_EQUAL;
    }
  }

  for (i = 0; t1[i] != NULL; i++) {
    free(t1[i]);
  }
  for (i = 0; t2[i] != NULL; i++) {
    free(t2[i]);
  }
  free(t1);
  free(t2);

  return result;
}

enum cvercmp_type cvercmp_cmp(const char *ver1, const char *ver2)
{
  enum cvercmp_type result = CVERCMP_EQUAL;
  bool solution = false;
  int i;
  char **tokens1 = cvercmp_ver_tokenize(ver1);
  char **tokens2 = cvercmp_ver_tokenize(ver2);

  for (i = 0; (tokens1[i] != NULL && tokens2[i] != NULL) && !solution; i++) {
    if (strcasecmp(tokens1[i], tokens2[i])) {
      /* Parts are not equal */
      result = cvercmp_tokens(tokens1[i], tokens2[i]);
      solution = true;
    }
  }

  if (!solution) {
    /* Ran out of tokens in one string. */
    result = cvercmp_tokens(tokens1[i], tokens2[i]);
  }

  for (i = 0; tokens1[i] != NULL; i++) {
    free(tokens1[i]);
  }
  for (i = 0; tokens2[i] != NULL; i++) {
    free(tokens2[i]);
  }
  free(tokens1);
  free(tokens2);

  return result;
}

static char **cvercmp_ver_tokenize(const char *ver)
{
  int num = 0;
  int idx;
  int verlen = strlen(ver);
  char **tokens;
  int i;
  int tokenlen;

  for (idx = 0; idx < verlen; idx += cvercmp_next_token(ver + idx) + 1) {
    num++;
  }

  tokens = calloc(sizeof(char *), num + 1);

  for (i = 0, idx = 0; i < num; i++) {
    tokenlen = cvercmp_next_token(ver + idx);
    tokens[i] = malloc(sizeof(char) * (tokenlen + 1));
    strncpy(tokens[i], ver + idx, tokenlen);
    tokens[i][tokenlen] = '\0';
    idx += tokenlen + 1; /* Skip character separating tokens. */
  }

  return tokens;
}

static int cvercmp_next_token(const char *str)
{
  int i;

  for (i = 0;
       str[i] != '\0' && str[i] != '.' && str[i] != '-' && str[i] != '_';
       i++) {
  }

  return i;
}

static char **cvercmp_ver_subtokenize(const char *ver)
{
  int num = 0;
  int idx;
  int verlen;
  char **tokens;
  int i;
  int tokenlen;

  /* Treat NULL string as empty string */
  if (!ver) {
      ver = "";
  }
  verlen = strlen(ver);

  for (idx = 0; idx < verlen; idx += cvercmp_next_subtoken(ver + idx)) {
    num++;
  }

  tokens = calloc(sizeof(char *), num + 1);

  for (i = 0, idx = 0; i < num; i++) {
    tokenlen = cvercmp_next_subtoken(ver + idx);
    tokens[i] = malloc(sizeof(char) * (tokenlen + 1));
    strncpy(tokens[i], ver + idx, tokenlen);
    tokens[i][tokenlen] = '\0';
    idx += tokenlen;
  }

  return tokens;
}

static int cvercmp_next_subtoken(const char *str)
{
  int i;
  bool alpha;
  int sc0 = str[0];
  int sci;

  if (isdigit(sc0)) {
    alpha = false;
  } else {
    alpha = true;
  }

  for (i = 0; sci = str[i],
         sci != '\0'
         && ((alpha && !isdigit(sci)) || (!alpha && isdigit(sci)));
       i++) {
  }

  return i;
}

static enum cvercmp_prever cvercmp_parse_prever(const char *ver)
{
  int i;

  for (i = 0; preverstrs[i].str != NULL; i++) {
    if (!strcasecmp(ver, preverstrs[i].str)) {
      return preverstrs[i].prever;
    }
  }

  return CVERCMP_PRE_NONE;
}
