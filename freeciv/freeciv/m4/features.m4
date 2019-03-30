# Macros to list feature set
#

AC_DEFUN([FC_FEATURE],
[
  if test "x$2" = "xmissing" ; then
    missing_list="${missing_list}
$1"
    if test "x$3" != "x" ; then
      missing_list="${missing_list} - requires $3"
    fi
  fi
])

# List features that were not enabled due to some missing dependency
AC_DEFUN([FC_MISSING_FEATURES],
[
AC_ARG_WITH([missinglist],
  AS_HELP_STRING([--with-missinglist], [list missing features after configure]),
  [list_missing_features=${withval}], [list_missing_features=no])

if test "x$list_missing_features" = "xyes" ; then
  FC_FEATURE([readline], [$feature_readline])
  FC_FEATURE([sound], [$feature_sound], [SDL_mixer])
  FC_FEATURE([additional mapimg formats], [$feature_magickwand], [MagickWand])
  FC_FEATURE([bz2 savegame compression], [$feature_bz2], [libbz2])
  FC_FEATURE([xz savegame compression], [$feature_xz], [liblzma])
  FC_FEATURE([threads suitable for threaded ai], [$feature_thr_cond], [pthreads])
  FC_FEATURE([lua linked from system], [$feature_syslua], [lua-5.3])
  FC_FEATURE([tolua command from system], [$feature_systolua_cmd], [tolua])
  FC_FEATURE([Ruleset Editor], [$feature_ruledit], [Qt5 development packages])

  if test "x$missing_list" = "x" ; then
    AC_MSG_NOTICE([
************** No optional features missing ***************])
  else
    AC_MSG_NOTICE([
**************** Missing optional features ****************
$missing_list])
  fi
fi
])
