#!/bin/sh

# winbuild.sh: Cross-compiling freeciv from linux to Windows using Crosser dllstack
#
# (c) 2008-2016 Marko Lindqvist
#
# This script is licensed under Gnu General Public License version 2 or later.
# See COPYING available from the same location you got this script.

# Version 2.3.2 (02-Jan-18)

WINBUILD_VERSION="2.3.2"
MIN_WINVER=0x0601 # Windows 7
CROSSER_FEATURE_LEVEL=1.6

if test "x$1" = x || test "x$1" = "x-h" || test "x$1" = "x--help" ; then
  echo "Usage: $0 <crosser dir> [gui]"
  exit 1
fi

if test "x$1" = "x-v" || test "x$1" = "x--version" ; then
  echo "winbuild.sh version $WINBUILD_VERSION"
  exit
fi

DLLSPATH="$1"

if ! test -d "$DLLSPATH" ; then
  echo "Dllstack directory \"$DLLSPATH\" not found!" >&2
  exit 1
fi

if ! test -f "$DLLSPATH/crosser.txt" ; then
  echo "Directory \"$DLLSPATH\" does not look like crosser environment!" >&2
  exit 1
fi

VERREV="$(../../fc_version)"
if test "x$INST_CROSS_MODE" != "xrelease" ; then
  if test -d ../../.git || test -f ../../.git ; then
    VERREV="$VERREV-$(cd ../.. && git rev-parse --short HEAD)"
  fi
fi

FLVL=$(grep "FeatureLevel=" $DLLSPATH/crosser.txt | sed -e 's/FeatureLevel="//' -e 's/"//')

if test "x$FLVL" != "x$CROSSER_FEATURE_LEVEL" ; then
  echo "Crosser feature level \"$FLVL\", required \"$CROSSER_FEATURE_LEVEL\"!"
  exit 1
fi

CSET=$(grep "Set=" $DLLSPATH/crosser.txt | sed -e 's/Set="//' -e 's/"//')

if test "x$CSET" != "xcurrent" ; then
  echo "Crosser set is \"$CSET\", only \"current\" is supported!"
  exit 1
fi

SETUP=$(grep "Setup=" $DLLSPATH/crosser.txt | sed -e 's/Setup="//' -e 's/"//')

if test "x$2" = "xruledit" ; then
  SINGLE_GUI=true
  GUIP="-ruledit"
  RULEDIT="yes"
  CLIENTS="no"
  FCMP="no"
  SERVER="no"
  AIS="--enable-ai-static=stub"
elif test "x$2" != "x" ; then
  SINGLE_GUI=true
  GUIP="-$2"
  SERVER="yes"
  if test "x$2" = "xqt" ; then
    RULEDIT="yes"
  else
    RULEDIT="no"
  fi
  CLIENTS="$2"
  case $2 in
    gtk3) FCMP="gtk3" ;;
    sdl2) FCMP="gtk3" ;;
    gtk3.22) FCMP="gtk3" ;;
    gtk3x) FCMP="gtk3" ;;
    qt) FCMP="qt" ;;
    *) echo "Unknown gui \"$2\"!" >&2
       exit 1 ;;
  esac
else
  GUIP=""
  SERVER="yes"
  RULEDIT="yes"
fi

if test "x$NAMEP" = "x" ; then
  NAMEP="$GUIP"
fi

if ! mkdir -p build-${SETUP}${NAMEP} ; then
  echo "Can't create build directory \"build-${SETUP}${NAMEP}\"!" >&2
  exit 1
fi

if test x$SETUP = xwin64 ; then
  TARGET=x86_64-w64-mingw32
  if test "x$SINGLE_GUI" != "xtrue" ; then
    CLIENTS="gtk3,sdl2,gtk3.22"
    FCMP="gtk3,cli"
  fi
  VERREV="win64-$VERREV"
else
  TARGET=i686-w64-mingw32
  if test "x$SINGLE_GUI" != "xtrue" ; then
    CLIENTS="gtk3,sdl2,gtk3.22"
    FCMP="gtk3,cli"
  fi
  VERREV="win32-$VERREV"
fi

if test "x$SINGLE_GUI" != "xtrue" ; then
  if grep "CROSSER_QT" $DLLSPATH/crosser.txt | grep yes > /dev/null
  then
    CLIENTS="$CLIENTS,qt"
    FCMP="$FCMP,qt"
  fi
fi

echo "----------------------------------"
echo "Building for $SETUP"
echo "Freeciv version $VERREV"
echo "Clients: $CLIENTS"
echo "----------------------------------"

export CC="$TARGET-gcc -static-libgcc -static-libstdc++"
export CXX="$TARGET-g++ -static-libgcc -static-libstdc++"

if ! ../../autogen.sh --no-configure-run ; then
  echo "Autogen failed" >&2
  exit 1
fi

INSTALL_DIR="$(pwd)/freeciv-${VERREV}${NAMEP}"

if ! (
cd build-${SETUP}${NAMEP}

if test "x$INST_CROSS_MODE" = "xsnapshot" ; then
  GITREVP="--enable-gitrev"
else
  GITREVP=""
fi

if ! ../../../configure FREECIV_LABEL_FORCE="<base>-crs" CPPFLAGS="-I${DLLSPATH}/include -D_WIN32_WINNT=${MIN_WINVER}" CFLAGS="-Wno-error" PKG_CONFIG_LIBDIR="${DLLSPATH}/lib/pkgconfig" --enable-sys-tolua-cmd --with-magickwand="${DLLSPATH}/bin" --prefix="/" $GITREVP --enable-client=$CLIENTS --enable-fcmp=$FCMP --enable-debug --host=$TARGET --build=$(../../../bootstrap/config.guess) --with-libiconv-prefix=${DLLSPATH} --with-sqlite3-prefix=${DLLSPATH} --with-followtag="crosser" --enable-crosser ${AIS} --disable-freeciv-manual --enable-sdl-mixer=sdl2 --with-qt5-includes=${DLLSPATH}/include --with-qt5-libs=${DLLSPATH}/lib --with-tinycthread --enable-server=$SERVER --enable-ruledit=$RULEDIT $EXTRA_CONFIG
then
  echo "Configure failed" >&2
  exit 1
fi

if ! make $MAKE_PARAMS DESTDIR="$INSTALL_DIR" clean install
then
  echo "Build failed" >&2
  exit 1
fi

if ! cp gen_headers/fc_config.h $INSTALL_DIR/share/freeciv/ ; then
  echo "Storing fc_config.h failed" >&2
  exit 1
fi

if ! cp $DLLSPATH/crosser.txt $INSTALL_DIR/share/freeciv/ ; then
  echo "Storing crosser.txt failed" >&2
  exit 1
fi

if ! cp $DLLSPATH/ComponentVersions.txt $INSTALL_DIR/share/freeciv/CrosserComponents.txt ; then
  echo "Storing CrosserComponents.txt failed" >&2
  exit 1
fi
) then
  exit 1
fi

if ! mkdir -p Output ; then
  echo "Creating Output directory failed" >&2
  exit 1
fi

if ! 7z a -r Output/freeciv-${VERREV}${NAMEP}.7z freeciv-${VERREV}${NAMEP}
then
  echo "7z failed" >&2
  exit 1
fi
