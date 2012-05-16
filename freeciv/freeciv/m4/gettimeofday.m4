dnl FC_CHECK_GETTIMEOFDAY_RUNTIME(EXTRA-LIBS, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
dnl
dnl This tests whether gettimeofday works at runtime.  Here, "works"
dnl means: time doesn't go backward and time doesn't jump forward by
dnl a huge amount. It seems that glibc 2.3.1 is broken in this respect.

AC_DEFUN([FC_CHECK_GETTIMEOFDAY_RUNTIME],
[
templibs="$LIBS"
LIBS="$1 $LIBS"
AC_TRY_RUN([
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define VERBOSE 0
#define SECONDS	3

int main(int argc, char **argv)
{
  struct timeval tv[2], start, end;
  int calls;

  if (gettimeofday(&start, NULL) == -1) {
    return 1;
  }
  end = start;
  end.tv_sec += SECONDS;

  tv[0] = start;
  tv[1] = start;

  for (calls = 0;; calls++) {
    time_t sec;

    if (gettimeofday(&tv[0], NULL) == -1) {
      return 1;
    }

    if (tv[0].tv_sec < tv[1].tv_sec) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: going backward by %lds\n", diff,
	     tv[1].tv_sec - tv[0].tv_sec);
#endif
      return 1;
    }

    if (tv[0].tv_sec == tv[1].tv_sec && tv[0].tv_usec < tv[1].tv_usec) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: going backward by %ldus\n", diff,
	     tv[1].tv_usec - tv[0].tv_usec);
#endif
      return 1;
    }

    if (tv[0].tv_sec > tv[1].tv_sec + 1) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: going forward by %lds\n", diff,
	     tv[0].tv_sec - tv[1].tv_sec);
#endif
      return 1;
    }

    sec = time(NULL);

    if (abs(sec - tv[0].tv_sec) > 1) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: time() = %ld, gettimeofday = %ld, diff = %ld\n", diff,
	     (long)sec, (long)tv[0].tv_sec, sec - (long)tv[0].tv_sec);
#endif
      return 1;
    }

    if (timercmp(&tv[0], &end, >)) {
      break;
    }
    tv[1] = tv[0];
  }

#if VERBOSE
  {
    double diff =
	(tv[1].tv_sec - start.tv_sec) +
	((tv[1].tv_usec - start.tv_usec) / 1e6);
    printf("%d calls in %fs = %fus/call\n", calls, diff, 1e6 * diff / calls);
  }
#endif
  return 0;
}
],
[AC_MSG_RESULT(yes)
  [$2]],
[AC_MSG_RESULT(no)
  [$3]],
[AC_MSG_RESULT(unknown: cross-compiling)
  [$3]])
LIBS="$templibs"
])
