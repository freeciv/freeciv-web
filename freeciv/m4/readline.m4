
dnl FC_CHECK_READLINE_RUNTIME(EXTRA-LIBS, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
dnl
dnl This tests whether readline works at runtime.  Here, "works"
dnl means "doesn't dump core", as some versions do if linked
dnl against wrong ncurses library.  Compiles with LIBS modified 
dnl to included -lreadline and parameter EXTRA-LIBS.
dnl Should already have checked that header and library exist.
dnl
AC_DEFUN([FC_CHECK_READLINE_RUNTIME],
[AC_MSG_CHECKING(whether readline works at runtime)
templibs="$LIBS"
LIBS="-lreadline $1 $LIBS"
AC_TRY_RUN([
/*
 * testrl.c
 * File revision 0
 * Check to make sure that readline works at runtime.
 * (Specifically, some readline packages link against a wrong 
 * version of ncurses library and dump core at runtime.)
 * (c) 2000 Jacob Lundberg, jacob@chaos2.org
 */

#include <stdio.h>
/* We assume that the presence of readline has already been verified. */
#include <readline/readline.h>
#include <readline/history.h>

/* Setup for readline. */
#define TEMP_FILE "./conftest.readline.runtime"

static void handle_readline_input_callback(char *line) {
/* Generally taken from freeciv-1.11.4/server/sernet.c. */
  if(line) {
    if(*line)
      add_history(line);
    /* printf(line); */
  }
}

int main(void) {
/* Try to init readline and see if it barfs. */
  using_history();
  read_history(TEMP_FILE);
  rl_initialize();
  rl_callback_handler_install("_ ", handle_readline_input_callback);
  rl_callback_handler_remove();  /* needed to re-set terminal */
  return(0);
}
],
[AC_MSG_RESULT(yes)
  [$2]],
[AC_MSG_RESULT(no)
  [$3]],
[AC_MSG_RESULT(unknown: cross-compiling)
  [$2]])
LIBS="$templibs"
])

AC_DEFUN([FC_HAS_READLINE],
[
    dnl Readline library and header files.
    if test "$WITH_READLINE" = "yes" || test "$WITH_READLINE" = "maybe"; then
       HAVE_TERMCAP="";
       dnl Readline header
       AC_CHECK_HEADER(readline/readline.h,
                       have_readline_header=1,
                       have_readline_header=0)
       if test "$have_readline_header" = "0"; then
           if test "$WITH_READLINE" = "yes"; then
               AC_MSG_ERROR(Did not find readline header file. 
You may need to install a readline \"development\" package.)
           else
               AC_MSG_WARN(Did not find readline header file. 
Configuring server without readline support.)
           fi
       else
           dnl Readline lib
           AC_CHECK_LIB(readline, completion_matches, 
                         have_readline_lib=1, have_readline_lib=0)
           dnl Readline lib >= 4.2
           AC_CHECK_LIB(readline, rl_completion_matches, 
                         have_new_readline_lib=1, have_new_readline_lib=0)
           if test "$have_readline_lib" != "1" && test "$have_new_readline_lib" != "1"; then
               dnl Many readline installations are broken in that they
               dnl don't set the dependency on the curses lib up correctly.
               dnl We give them a hand by trying to guess what might be needed.
               dnl
               dnl Some older Unices may need both -lcurses and -ltermlib,
               dnl but we don't support that just yet.  This check will take
               dnl the first lib that it finds and just link to that.
               AC_CHECK_LIB(tinfo, tgetent, HAVE_TERMCAP="-ltinfo",
                 AC_CHECK_LIB(ncurses, tgetent, HAVE_TERMCAP="-lncurses",
                   AC_CHECK_LIB(curses, tgetent, HAVE_TERMCAP="-lcurses",
                     AC_CHECK_LIB(termcap, tgetent, HAVE_TERMCAP="-ltermcap",
                       AC_CHECK_LIB(termlib, tgetent, HAVE_TERMCAP="-ltermlib")
                     )
                   )
                 )
               )

               if test x"$HAVE_TERMCAP" != "x"; then
                   dnl We can't check for completion_matches() again,
                   dnl cause the result is cached. And autoconf doesn't
                   dnl seem to have a way to uncache it.
                   AC_CHECK_LIB(readline, filename_completion_function,
                         have_readline_lib=1, have_readline_lib=0,
                        "$HAVE_TERMCAP")
                   if test "$have_readline_lib" = "1"; then
                       AC_MSG_WARN(I had to manually add $HAVE_TERMCAP dependency to 
make readline library pass the test.)
                   fi
                   dnl We can't check for rl_completion_matches() again,
                   dnl cause the result is cached. And autoconf doesn't
                   dnl seem to have a way to uncache it.
                   AC_CHECK_LIB(readline, rl_filename_completion_function,
                         have_new_readline_lib=1, have_new_readline_lib=0,
                        "$HAVE_TERMCAP")
                   if test "$have_new_readline_lib" = "1"; then
                       AC_MSG_WARN(I had to manually add $HAVE_TERMCAP dependency to 
make readline library pass the test.)
                   fi
               fi
           fi

           if test "$have_new_readline_lib" = "1"; then
               FC_CHECK_READLINE_RUNTIME($HAVE_TERMCAP,
                         have_new_readline_lib=1, have_new_readline_lib=0)
               if test "$have_new_readline_lib" = "1"; then
                   SERVER_LIBS="-lreadline $SERVER_LIBS $HAVE_TERMCAP"
                   AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE, 1, [Readline support])
                   AC_DEFINE_UNQUOTED(HAVE_NEWLIBREADLINE, 1, [Modern readline])
               else
                   if test "$WITH_READLINE" = "yes"; then
                       AC_MSG_ERROR(Specified --with-readline but the 
runtime test of readline failed.)
                   else
                       AC_MSG_WARN(Runtime test of readline failed. 
Configuring server without readline support.)
                   fi
               fi
           else
               if test "$have_readline_lib" = "1"; then
                   FC_CHECK_READLINE_RUNTIME($HAVE_TERMCAP,
                       have_readline_lib=1, have_readline_lib=0)
                   if test "$have_readline_lib" = "1"; then
                       SERVER_LIBS="-lreadline $SERVER_LIBS $HAVE_TERMCAP"
                       AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE, 1, [Readline support])
                   else
                       if test "$WITH_READLINE" = "yes"; then
                           AC_MSG_ERROR(Specified --with-readline but the 
runtime test of readline failed.)
                       else
                           AC_MSG_WARN(Runtime test of readline failed. 
Configuring server without readline support.)
                       fi
                   fi
               else
                   if test "$WITH_READLINE" = "yes"; then
                       AC_MSG_ERROR(Specified --with-readline but the 
test to link against the library failed.)
                   else
                       AC_MSG_WARN(Test to link against readline library failed. 
Configuring server without readline support.)
                   fi
               fi
           fi
       fi
    fi
])
