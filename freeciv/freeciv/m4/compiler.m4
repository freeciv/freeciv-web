# Macros to check compiler options
#

# Helper function that adds flags (words) to variable listing them.
# Makes sure there is no extra spaces in any situation
#
# $1 - Name of the target variable
# $2 - Flags to add
#
AC_DEFUN([FC_ADD_WORDS_TO_VAR],
[
old_value="`eval echo '$'$1`"
if test "x$old_value" = "x" ; then
  $1="$2"
elif test "x$2" != "x" ; then
  $1="$old_value $2"
fi
])

# Check if compiler supports given commandline parameter in language specific
# variable. If it does, it will be concatenated to variable. If several
# parameters are given, they are tested, and added to target variable,
# one at a time.
#
# $1 - Language
# $2 - Language specific variable
# $3 - Parameters to test
# $4 - Additional parameters
# $5 - Variable where to add
#

AC_DEFUN([FC_COMPILER_FLAGS],
[
AC_LANG_PUSH([$1])

flags_save="`eval echo '$'$2`"
accepted_flags=""

for flag in $3
do
  $2="$flags_save $accepted_flags $flag $4"
  AC_COMPILE_IFELSE([int a;],
                    [FC_ADD_WORDS_TO_VAR([accepted_flags], [$flag])])
done
FC_ADD_WORDS_TO_VAR([$5], [$accepted_flags])

$2="$flags_save"

AC_LANG_POP([$1])
])

# Commandline flag tests for C and C++
#
#
# $1 - Parameters to test
# $2 - Additional parameters
# $3 - Variable where to add

AC_DEFUN([FC_C_FLAGS],
[
FC_COMPILER_FLAGS([C], [CFLAGS], [$1], [$2], [$3])
])


AC_DEFUN([FC_CXX_FLAGS],
[
FC_COMPILER_FLAGS([C++], [CXXFLAGS], [$1], [$2], [$3])
])

# Commandline flag tests for linker
#
#
# $1 - Parameters to test
# $2 - Additional parameters
# $3 - Variable where to add
AC_DEFUN([FC_LD_FLAGS],
[
flags_save=$LDFLAGS
accepted_flags=""

for flag in $1
do
  LDFLAGS="$flags_save $accepted_flags $flag $2"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([], [int a;])],
                 [FC_ADD_WORDS_TO_VAR([accepted_flags], [$flag])])
done
FC_ADD_WORDS_TO_VAR([$3], [$accepted_flags])

LDFLAGS="$flags_save"
])
