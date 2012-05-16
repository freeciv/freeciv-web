dnl AM_ALSA_SUPPORT([ACTION-IF-SUPPORTS [, ACTION-IF-NOT-SUPPORTS]])
dnl Partially stolen from alsaplayer
dnl
AC_DEFUN([AM_ALSA_SUPPORT],
[dnl
  AC_MSG_CHECKING(for ALSA version)
  AC_EGREP_CPP([AP_maGiC_VALUE],
  [
#include <sys/asoundlib.h>
#if defined(SND_LIB_MAJOR) && defined(SND_LIB_MINOR)
#if SND_LIB_MAJOR>1 || (SND_LIB_MAJOR==1 && SND_LIB_MINOR>=0)
AP_maGiC_VALUE
#endif
#endif
  ],
  AC_MSG_RESULT([>= 1.0])
  AC_MSG_CHECKING(for Audio File Library version)
  AC_EGREP_CPP([AP_maGiC_VALUE],
  [
#include <audiofile.h>
#if defined(LIBAUDIOFILE_MAJOR_VERSION) && defined(LIBAUDIOFILE_MINOR_VERSION)
#if LIBAUDIOFILE_MAJOR_VERSION>0 || (LIBAUDIOFILE_MAJOR_VERSION==0 && LIBAUDIOFILE_MINOR_VERSION>=2)
AP_maGiC_VALUE
#endif
#endif
  ],
  AC_MSG_RESULT([>= 0.2])
  ALSA_CFLAGS=""
  ALSA_LIB="-laudiofile -lasound"
  ifelse([$1], , :, [$1]),
  AC_MSG_RESULT([no])
  ifelse([$2], , :, [$2])
  ),
  AC_MSG_RESULT([no])
  ifelse([$2], , :, [$2])
  )
])
