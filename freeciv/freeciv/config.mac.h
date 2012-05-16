/* config.mac.h.  converted from config.h.in by Andy Black. */
#ifndef FC_CONFIG_H
#define FC_CONFIG_H

#include <MacHeaders.h>
#include <ansi_prefix.mac.h> /*needed in time.h and ansi_files.c*/

/* Define if using alloca.c.  */
#undef C_ALLOCA

/* Define to empty if the keyword does not work.  */
/*#undef const*/ /*not needed?*/

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
#undef CRAY_STACKSEG_END

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H

/* Define if you don't have vprintf but do have _doprnt.  */
#undef HAVE_DOPRNT

/* Define if you have a working `mmap' system call.  */
#undef HAVE_MMAP

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF

/* Define as __inline if that's what the C compiler calls it.  */
/*#undef inline*/ /* not needed?*/

/* Define to `long' if <sys/types.h> doesn't define.  */
#define off_t long

/* Define if you need to in order for stat and other things to work.  */
#undef _POSIX_SOURCE

/* Define as the return type of signal handlers (int or void).  */
#undef RETSIGTYPE

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/*#undef size_t*/ /*not needed ?*/

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
#define STACK_DIRECTION -1

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#undef TIME_WITH_SYS_TIME

/* Define if your <sys/time.h> declares struct tm.  */
#undef TM_IN_SYS_TIME

/* Define if your <time.h> declares struct tm.  */
#define TM_IN_TIME

/* Define if the X Window System is missing or not being used.  */
#define X_DISPLAY_MISSING

#undef MAJOR_VERSION
#undef MINOR_VERSION
#undef PATCH_VERSION
#undef VERSION_LABEL
#undef IS_DEVEL_VERSION
#undef IS_BETA_VERSION
#undef VERSION_STRING
#undef HAVE_LIBICE
#undef HAVE_LIBSM
#undef HAVE_LIBX11
#undef HAVE_LIBXAW
#undef HAVE_LIBXAW3D
#undef HAVE_LIBXEXT
#undef HAVE_LIBXMU
#undef HAVE_LIBXPM
#undef HAVE_LIBXT
#undef ENABLE_NLS
#undef HAVE_CATGETS
#undef HAVE_GETTEXT
#undef HAVE_LC_MESSAGES
#undef HAVE_STPCPY
#undef LOCALEDIR
#undef DEFAULT_DATA_PATH
#undef HAVE_SIGPIPE
#undef XPM_H_NO_X11
#undef FUNCPROTO
#undef NARROWPROTO
#undef HAVE_LIBREADLINE
#define ALWAYS_ROOT 1 /*may not be needed.  Metroworks provides dummy functions*/

/* copland Compatibility (os x?), _MAC ONLY_ (pointless on other systems) */
#undef STRICT_WINDOWS

#define STRICT_WINDOWS 1 /* seq used to enable strict windows */

/* copland Compatibility (os x?), _MAC ONLY_ (pointless on other systems) */
#undef STRICT_CONTROLS

#define STRICT_CONTROLS 1 /* seq used to enable strict controls */
#undef STRICT_MENUS/* copland Compatibility? (os x?), _MAC ONLY_ (pointless on other systems)*/
#define STRICT_MENUS 1 /* seq used to enable strict menus */
#define GENERATING_MAC /*use for mac native code*/
#define HAVE_OPENTRANSPORT /*used for OpenTransport Networking*/
#undef PATH_SEPARATOR
#define SOCKET_ZERO_ISNT_STDIN 1
#undef NONBLOCKING_SOCKETS
#undef HAVE_FCNTL
#undef HAVE_IOCTL
#undef OPTION_FILE_NAME

/* Define if you have the __argz_count function.  */
#undef HAVE___ARGZ_COUNT

/* Define if you have the __argz_next function.  */
#undef HAVE___ARGZ_NEXT

/* Define if you have the __argz_stringify function.  */
#undef HAVE___ARGZ_STRINGIFY

/* Define if you have the dcgettext function.  */
#undef HAVE_DCGETTEXT

/* Define if you have the fdopen function.  */
#undef HAVE_FDOPEN

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD

/* Define if you have the gethostname function.  */
#undef HAVE_GETHOSTNAME

/* Define if you have the getpagesize function.  */
#undef HAVE_GETPAGESIZE

/* Define if you have the getpwuid function.  */
#undef HAVE_GETPWUID

/* Define if you have the gettimeofday function.  */
#undef HAVE_GETTIMEOFDAY

/* Define if you have the munmap function.  */
#undef HAVE_MUNMAP

/* Define if you have the putenv function.  */
#undef HAVE_PUTENV

/* Define if you have the select function.  */
#undef HAVE_SELECT

/* Define if you have the setenv function.  */
#undef HAVE_SETENV

/* Define if you have the setlocale function.  */
#define HAVE_SETLOCALE

/* Define if you have the snooze function.  */
#undef HAVE_SNOOZE

/* Define if you have the stpcpy function.  */
#undef HAVE_STPCPY

/* Define if you have the strcasecmp function.  */
#undef HAVE_STRCASECMP

/* Define if you have the strchr function.  */
#define HAVE_STRCHR

/* Define if you have the strdup function.  */
#undef HAVE_STRDUP

/* Define if you have the strerror function.  */
#define HAVE_STRERROR

/* Define if you have the strlcat function.  */
#undef HAVE_STRLCAT

/* Define if you have the strlcpy function.  */
#undef HAVE_STRLCPY

/* Define if you have the strncasecmp function.  */
#undef HAVE_STRNCASECMP

/* Define if you have the strstr function.  */
#define HAVE_STRSTR

/* Define if you have the usleep function.  */
#undef HAVE_USLEEP

/* Define if you have the vsnprintf function.  */
#undef HAVE_VSNPRINTF

/* Define if you have the <argz.h> header file.  */
#undef HAVE_ARGZ_H

/* Define if you have the <arpa/inet.h> header file.  */
#undef HAVE_ARPA_INET_H

/* Define if you have the <fcntl.h> header file.  */
#undef HAVE_FCNTL_H

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H

/* Define if you have the <locale.h> header file.  */
#define HAVE_LOCALE_H

/* Define if you have the <malloc.h> header file.  */
#undef HAVE_MALLOC_H

/* Define if you have the <netdb.h> header file.  */
#undef HAVE_NETDB_H

/* Define if you have the <netinet/in.h> header file.  */
#undef HAVE_NETINET_IN_H

/* Define if you have the <nl_types.h> header file.  */
#undef HAVE_NL_TYPES_H

/* Define if you have the <pwd.h> header file.  */
#undef HAVE_PWD_H

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H

/* Define if you have the <sys/ioctl.h> header file.  */
#undef HAVE_SYS_IOCTL_H

/* Define if you have the <sys/param.h> header file.  */
#undef HAVE_SYS_PARAM_H

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the <sys/signal.h> header file.  */
#undef HAVE_SYS_SIGNAL_H

/* Define if you have the <sys/socket.h> header file.  */
#undef HAVE_SYS_SOCKET_H

/* Define if you have the <sys/termio.h> header file.  */
#undef HAVE_SYS_TERMIO_H

/* Define if you have the <sys/time.h> header file.  */
#undef HAVE_SYS_TIME_H

/* Define if you have the <sys/types.h> header file.  */
#undef HAVE_SYS_TYPES_H

/* Define if you have the <sys/uio.h> header file.  */
#undef HAVE_SYS_UIO_H

/* Define if you have the <termios.h> header file.  */
#undef HAVE_TERMIOS_H

/* Define if you have the <unistd.h> header file.  */
#undef HAVE_UNISTD_H	/*conflicts with ot*/

/* Define if you have the i library (-li).  */
#undef HAVE_LIBI

/* Define if you have the nls library (-lnls).  */
#undef HAVE_LIBNLS

/* Define if you have the z library (-lz).  */
#undef HAVE_LIBZ

/* Name of package */
#undef PACKAGE

/* Version number of package */
#undef VERSION


#endif /* FC_CONFIG_H */
