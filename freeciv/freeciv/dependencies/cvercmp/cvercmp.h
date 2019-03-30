/*********************************************************
*                                                        *
* (c) 2011-2015 Marko Lindqvist                          *
*                                                        *
* Version contributed to Freeciv has been made available *
* under GNU Public License; either version 2, or         *
* (at your option) any later version.                    *
*                                                        *
*********************************************************/

#ifndef H_CVERCMP
#define H_CVERCMP

#include <stdbool.h>

enum cvercmp_type
{
  CVERCMP_EQUAL = 0,
  CVERCMP_NONEQUAL,
  CVERCMP_GREATER,
  CVERCMP_LESSER,
  CVERCMP_MIN,
  CVERCMP_MAX
};

bool cvercmp(const char *ver1, const char *ver2, enum cvercmp_type type);
enum cvercmp_type cvercmp_cmp(const char *ver1, const char *ver2);

bool cvercmp_equal(const char *ver1, const char *ver2);
bool cvercmp_nonequal(const char *ver1, const char *ver2);
bool cvercmp_greater(const char *ver1, const char *ver2);
bool cvercmp_lesser(const char *ver1, const char *ver2);
bool cvercmp_min(const char *ver1, const char *ver2);
bool cvercmp_max(const char *ver1, const char *ver2);

#endif /* H_CVERCMP */
