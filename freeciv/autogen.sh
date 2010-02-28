#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# This is a kludge to make Gentoo behave and select the 
# correct version of automake to use.
WANT_AUTOMAKE=1.8
export WANT_AUTOMAKE

DIE=0
package=freeciv
srcfile=client/civclient.c

SRCDIR=`dirname $0`
BUILDDIR=`pwd`

# Uncomment the line below to debug this file
#DEBUG=defined

FC_USE_NLS=yes
FC_HELP=no
FC_RUN_CONFIGURE=yes

# Leave out NLS checks
for NAME in $@ ; do
  if [ "x$NAME" = "x--help" ]; then 
    FC_HELP=yes
  fi
  if [ "x$NAME" = "x--disable-nls" ]; then 
    echo "! nls checks disabled"
    FC_USE_NLS=no
  fi
  if [ "x$NAME" = "x--no-configure-run" ]; then 
    FC_RUN_CONFIGURE=no
  fi
  FC_NEWARGLINE="$FC_NEWARGLINE $NAME"
done

debug ()
# print out a debug message if DEBUG is a defined variable
{
  if [ ! -z "$DEBUG" ]; then
    echo "DEBUG: $1"
  fi
}

real_package_name ()
# solve a real name of suitable package 
# first argument : package name (executable)
# second argument : source download url
# rest of arguments : major, minor, micro version
{
  RPACKAGE=$1
  RURL=$2
  RMAJOR=$3
  RMINOR=$4
  RMICRO=$5
  
  new_pkg=$RPACKAGE

  # check if given package is suitable
  if version_check 2 $RPACKAGE $RPACKAGE $RURL $RMAJOR $RMINOR $RMICRO; then
    version_check 1 $RPACKAGE $RPACKAGE $RURL $RMAJOR $RMINOR $RMICRO
  else
    # given package was too old or not available
    # search for the newest one
    if version_search $RPACKAGE; then
      if version_check 2 $RPACKAGE $new_pkg $RURL $RMAJOR $RMINOR $RMICRO; then
        # suitable package found
        version_check 1 $RPACKAGE $new_pkg $RURL $RMAJOR $RMINOR $RMICRO
      else
        # the newest package is not new enough or it is broken
        version_check 1 $RPACKAGE $RPACKAGE $RURL $RMAJOR $RMINOR $RMICRO
        return 1
      fi
    else 
      # no version of given package with version information 
      # in its name available
      version_check 1 $RPACKAGE $RPACKAGE $RURL $RMAJOR $RMINOR $RMICRO
      return 1
    fi
  fi

  REALPKGNAME=$new_pkg
}

version_search ()
# search the newest version of a package 
# first argument : package name (executable)
{
  SPACKAGE=$1
  STOREDIFS=$IFS
  IFS=":"
  set -- $PATH
  IFS=$STOREDIFS
  
  s_pkg_major=0
  s_pkg_minor=0
  new_pkg=
  
  for SEARCHDIR ; do
    for MATCHSTUFF in `ls "$SEARCHDIR/$SPACKAGE-"* 2> /dev/null` ; do
      for FOUNDPKG in $MATCHSTUFF; do 
        # parse version information from name
        new_s_pkg_major=`echo $FOUNDPKG | cut -s -d- -f2 | cut -s -d. -f1`
        new_s_pkg_minor=`echo $FOUNDPKG | cut -s -d- -f2 | cut -s -d. -f2`

        CORRECT=
        # check if version numbers are integers
        [ ! "x$new_s_pkg_major" = "x" ] && \
        [ "x`echo $new_s_pkg_major | sed s/[0-9]*//g`" = "x" ] && \
        [ ! "x$new_s_pkg_minor" = "x" ] && \
        [ "x`echo $new_s_pkg_minor | sed s/[0-9]*//g`" = "x" ] && \
        CORRECT=1

        if [ ! -z $CORRECT ]; then        
          if [ "$new_s_pkg_major" -gt "$s_pkg_major" ]; then
            s_pkg_major=$new_s_pkg_major
            s_pkg_minor=$new_s_pkg_minor
            new_pkg="$FOUNDPKG"
          elif [ "$new_s_pkg_major" -eq "$s_pkg_major" ]; then
            if [ "$new_s_pkg_minor" -gt "$s_pkg_minor" ]; then
              s_pkg_major=$new_s_pkg_major
              s_pkg_minor=$new_s_pkg_minor
              new_pkg="$FOUNDPKG"
            fi
          fi
        fi
      done
    done
  done
  
  if [ -z "$new_pkg"  ]; then
    return 1
  else
    return 0
  fi
}

version_check ()
# check the version of a package
# first argument : silent ('2'), complain ('1') or not ('0')
# second argument : package name 
# third argument : package name (executable)
# fourth argument : source download url
# rest of arguments : major, minor, micro version
{
  COMPLAIN=$1
  PACKAGEMSG=$2
  PACKAGE=$3
  URL=$4
  MAJOR=$5
  MINOR=$6
  MICRO=$7

  WRONG=

  debug "major $MAJOR minor $MINOR micro $MICRO"
  VERSION=$MAJOR
  if [ ! -z "$MINOR" ]; then VERSION=$VERSION.$MINOR; else MINOR=0; fi
  if [ ! -z "$MICRO" ]; then VERSION=$VERSION.$MICRO; else MICRO=0; fi

  debug "version $VERSION"
  if [ "$COMPLAIN" -ne "2" ]; then
    echo "+ checking for $PACKAGEMSG >= $VERSION ... " | tr -d '\n'
  fi
  
  ($PACKAGE --version) < /dev/null > /dev/null 2>&1 || 
  {
    if [ "$COMPLAIN" -ne "2" ]; then
      echo
      echo "You must have $PACKAGEMSG installed to compile $package."
      echo "Download the appropriate package for your distribution,"
      echo "or get the source tarball at $URL"
    fi
    return 1
  }
    
  # the following line is carefully crafted sed magic
  pkg_version=`$PACKAGE --version|head -n 1|sed 's/([^)]*)//g;s/^[a-zA-Z\.\ \-]*//;s/ .*$//'`
  debug "pkg_version $pkg_version"
  pkg_major=`echo $pkg_version | cut -d. -f1`
  pkg_minor=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
  pkg_micro=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
  [ -z "$pkg_minor" ] && pkg_minor=0
  [ -z "$pkg_micro" ] && pkg_micro=0

  debug "found major $pkg_major minor $pkg_minor micro $pkg_micro"

  #start checking the version
  if [ "$pkg_major" -lt "$MAJOR" ]; then
    WRONG=1
  elif [ "$pkg_major" -eq "$MAJOR" ]; then
    if [ "$pkg_minor" -lt "$MINOR" ]; then
      WRONG=1
    elif [ "$pkg_minor" -eq "$MINOR" -a "$pkg_micro" -lt "$MICRO" ]; then
      WRONG=1
    fi
  fi

  if [ ! -z "$WRONG" ]; then
   if [ "$COMPLAIN" -ne "2" ]; then
     echo "found $pkg_version, not ok !"
   fi
   if [ "$COMPLAIN" -eq "1" ]; then
     echo
     echo "You must have $PACKAGEMSG $VERSION or greater to compile $package."
     echo "Get the latest version from <$URL>."
     echo
   fi
   return 1
  else
    if [ "$COMPLAIN" -ne "2" ]; then
      echo "found $pkg_version, ok."
    fi
  fi
}

# Chdir to the srcdir, then run auto* tools.
cd $SRCDIR

[ -f $srcfile ] || {
  echo "Are you sure $SRCDIR is a valid source directory?"
  exit 1
}

# Check against acinclude.m4, which earlier versions used to generate.
[ -f acinclude.m4 ] && {
  echo "There is file acinclude.m4 in source directory."
  echo "It's probably left from earlier Freeciv build, and can cause"
  echo "unexpected results. Please remove it."
  exit 1
}

# autoconf and autoheader version numbers must be kept in sync
real_package_name "autoconf" "ftp://ftp.gnu.org/pub/gnu/autoconf/" 2 55 || DIE=1
AUTOCONF=$REALPKGNAME
real_package_name "autoheader" "ftp://ftp.gnu.org/pub/gnu/autoconf/" 2 55 || DIE=1
AUTOHEADER=$REALPKGNAME

# automake and aclocal version numbers must be kept in sync
real_package_name "automake" "ftp://ftp.gnu.org/pub/gnu/automake/" 1 6 || DIE=1
AUTOMAKE=$REALPKGNAME
real_package_name "aclocal" "ftp://ftp.gnu.org/pub/gnu/automake/" 1 6 || DIE=1
ACLOCAL=$REALPKGNAME
real_package_name "libtoolize" "ftp://ftp.gnu.org/pub/gnu/libtool/" 1 4 3 || DIE=1
LIBTOOLIZE=$REALPKGNAME

if [ "$FC_USE_NLS" = "yes" ]; then
  DIE2=0
  version_check 1 "xgettext" "xgettext" "ftp://ftp.gnu.org/pub/gnu/gettext/" 0 10 36 || DIE2=1
  version_check 1 "msgfmt" "msgfmt" "ftp://ftp.gnu.org/pub/gnu/gettext/" 0 10 36 || DIE2=1
  if [ "$DIE2" -eq 1 ]; then
    echo 
    echo "You may want to use --disable-nls to disable NLS."
    echo "This will also remove the dependency for xgettext and msgfmt."
    DIE=1
  fi
fi

if [ "$DIE" -eq 1 ]; then
  exit 1
fi

echo "+ running $ACLOCAL ..."
$ACLOCAL -I m4 -I dependencies/m4 $ACLOCAL_FLAGS || {
  echo
  echo "$ACLOCAL failed - check that all needed development files are present on system"
  exit 1
}

echo "+ running $AUTOHEADER ... "
$AUTOHEADER || {
  echo
  echo "$AUTOHEADER failed"
  exit 1
}
echo "+ running $AUTOCONF ... "
$AUTOCONF || {
  echo
  echo "$AUTOCONF failed"
  exit 1
}
echo "+ running $LIBTOOLIZE ... "
$LIBTOOLIZE -f || {
  echo
  echo "$LIBTOOLIZE failed"
  exit 1
}
echo "+ running $AUTOMAKE ... "
$AUTOMAKE -a -c || {
  echo
  echo "$AUTOMAKE failed"
  exit 1
}

# Chdir back to the builddir before the configure step.
cd $BUILDDIR

# now remove the cache, because it can be considered dangerous in this case
echo "+ removing config.cache ... "
rm -f config.cache

# exit if we did --no-configure-run
if [ "$FC_RUN_CONFIGURE" = "no" ]; then
  echo
  echo "Now type 'configure' to configure $package."
  exit 0
fi
 
echo "+ running configure ... "
echo
if [ -z "$FC_NEWARGLINE" ]; then
  echo "I am going to run ./configure with no arguments - if you wish "
  echo "to pass any to it, please specify them on the $0 command line."
else
  echo "using: $FC_NEWARGLINE"
fi
echo

$SRCDIR/configure $FC_NEWARGLINE || {
  echo
  echo "configure failed"
  exit 1
}

# abort if we did --help
if [ "$FC_HELP" = "yes" ]; then
  exit 1
fi

echo 
echo "Now type 'make' to compile $package."

exit 0
