#!/bin/bash

# Freeciv server version upgrade notes (backports)
# ------------------------------------------------
# osdn #????? is ticket in freeciv.org tracker:
# https://osdn.net/projects/freeciv/ticket/?????
#
# 0027-Make-research-invention-array-big-enough-for-A_FUTUR.patch
#   Fix out-of-bounds bug
#   osdn #47574
# 0032-Fix-freeciv-manual-failure-with-native_bases-cache.patch
#   Fix freeciv-manual failure
#   osdn #47607
# 0040-Fix-threaded-saving-of-the-game-on-signal.patch
#   Dataloss avoidance
#   osdn #47540
# 0015-lua_command-Use-fc_stat-instead-of-opening-the-file.patch
#   File descriptor leak fix
#   osdn #47609
# 0036-Optimize-V_RADIUS-usage.patch
#   Performance improvement
#   osdn #45627
# 0033-Improve-handling-of-fc_rand-1.patch
#   Performance improvement
#   osdn #45917
# 0027-Path-Finding-Make-MC-and-EC-unsigned-everywhere.patch
#   Path Findig fix
#   osdn #47731
# 0030-AI-Fix-check-if-new-building-enables-disables-action.patch
#   AI fix
#   osdn #42169
# 0031-Fix-Out-of-Bounds-write-to-bv_techs-bitvector.patch
#   Illegal memory access fix
#   osdn #47762
# 0046-Building-Advisor-Handle-wants-as-adv_want.patch
#   AI fix
#   osdn #47776
# 0032-Mapgenerator-Check-if-lake-exist-before-regenerate.patch
#   Zero-length VLA fix
#   osdn #47825
# 0017-Meson-Add-testmatic-support.patch
#   Debugging support improvement
#   osdn #47675
# 0050-Add-ERM_CLEAN.patch
#   Dependency for "Clean" action to work properly
#   Heavily rebased to current freeciv-web version when backported
#   osdn #47839
# 0024-Fix-cargo_iter_next-out-of-bounds-read.patch
#   Fix to illegal memory access
#   osdn #47900
# 0028-Meson-Stop-using-deprecated-get_cross_property.patch
#   Meson deprecation fix
#   osdn #44913

# Not in the upstream Freeciv server
# ----------------------------------
# meson_webperimental installs webperimental ruleset
# freeciv_segfauls_fix is a workaround some segfaults in the Freeciv server. Freeciv bug #23884.
# message_escape is a patch for protecting against script injection in the message texts.
# tutorial_ruleset changes the ruleset of the tutorial to one supported by Freeciv-web.
#      - This should be replaced by modification of the tutorial scenario that allows it to
#        work with multiple rulesets (Requires patch #7362 / SVN r33159)
# webgl_vision_cheat_temporary is a temporary solution to reveal terrain types to the WebGL client.
# longturn implements a very basic longturn mode for Freeciv-web.
# load_command_confirmation adds a log message which confirms that loading is complete, so that Freeciv-web can issue additional commands.
# endgame-mapimg is used to generate a mapimg at endgame for hall of fame.

# Local patches
# -------------
# Finally patches from patches/local are applied. These can be used
# to easily apply a temporary debug change that's not meant ever to get
# included to the repository.

declare -a GIT_PATCHLIST=(
)

declare -a PATCHLIST=(
  "backports/0027-Make-research-invention-array-big-enough-for-A_FUTUR"
  "backports/0032-Fix-freeciv-manual-failure-with-native_bases-cache"
  "backports/0040-Fix-threaded-saving-of-the-game-on-signal"
  "backports/0015-lua_command-Use-fc_stat-instead-of-opening-the-file"
  "backports/0036-Optimize-V_RADIUS-usage"
  "backports/0033-Improve-handling-of-fc_rand-1"
  "backports/0027-Path-Finding-Make-MC-and-EC-unsigned-everywhere"
  "backports/0030-AI-Fix-check-if-new-building-enables-disables-action"
  "backports/0031-Fix-Out-of-Bounds-write-to-bv_techs-bitvector"
  "backports/0046-Building-Advisor-Handle-wants-as-adv_want"
  "backports/0032-Mapgenerator-Check-if-lake-exist-before-regenerate"
  "backports/0017-Meson-Add-testmatic-support"
  "backports/rebased-0050-Add-ERM_CLEAN"
  "backports/0024-Fix-cargo_iter_next-out-of-bounds-read"
  "backports/0028-Meson-Stop-using-deprecated-get_cross_property"
  "meson_webperimental"
  "metachange"
  "text_fixes"
  "freeciv-svn-webclient-changes"
  "goto_fcweb"
  "tutorial_ruleset"
  "savegame"
  "maphand_ch"
  "server_password"
  "message_escape"
  "freeciv_segfauls_fix"
  "scorelog_filenames"
  "longturn"
  "load_command_confirmation"
  "webgl_vision_cheat_temporary"
  "endgame-mapimg"
  $(ls -1 patches/local/*.patch 2>/dev/null | sed -e 's,patches/,,' -e 's,\.patch,,' | sort)
)

apply_git_patch() {
  echo "*** Applying $1.patch ***"
  if ! git -C freeciv apply ../patches/$1.patch ; then
    echo "APPLYING PATCH $1.patch FAILED!"
    return 1
  fi
  echo "=== $1.patch applied ==="
}

apply_patch() {
  echo "*** Applying $1.patch ***"
  if ! patch -u -p1 -d freeciv < patches/$1.patch ; then
    echo "APPLYING PATCH $1.patch FAILED!"
    return 1
  fi
  echo "=== $1.patch applied ==="
}

# APPLY_UNTIL feature is used when rebasing the patches, and the working directory
# is needed to get to correct patch level easily.
if test "$1" != "" ; then
  APPLY_UNTIL="$1"
  au_found=false

  for patch in "${GIT_PATCHLIST[@]} ${PATCHLIST[@]}"
  do
    if test "$patch" = "$APPLY_UNTIL" ; then
      au_found=true
      APPLY_UNTIL="${APPLY_UNTIL}.patch"
    elif test "${patch}.patch" = "$APPLY_UNTIL" ; then
      au_found=true
    fi
  done
  if test "$au_found" != "true" ; then
    echo "There's no such patch as \"$APPLY_UNTIL\"" >&2
    exit 1
  fi
else
  APPLY_UNTIL=""
fi

. ./version.txt

CAPSTR_EXPECT="NETWORK_CAPSTRING=\"${ORIGCAPSTR}\""
CAPSTR_SRC="freeciv/fc_version"
echo "Verifying ${CAPSTR_EXPECT}"

if ! grep "$CAPSTR_EXPECT" ${CAPSTR_SRC} 2>/dev/null >/dev/null ; then
  echo "   Found  $(grep 'NETWORK_CAPSTRING=' ${CAPSTR_SRC}) in $(pwd)/freeciv/fc_version" >&2
  echo "Capstring to be replaced does not match that given in version.txt" >&2
  exit 1
fi

sed "s/${ORIGCAPSTR}/${WEBCAPSTR}/" freeciv/fc_version > freeciv/fc_version.tmp
mv freeciv/fc_version.tmp freeciv/fc_version
chmod a+x freeciv/fc_version

if test "$GIT_PATCHING" = "yes" ; then
  for patch in "${GIT_PATCHLIST[@]}"
  do
    if test "${patch}.patch" = "$APPLY_UNTIL" ; then
      echo "$patch not applied as requested to stop"
      break
    fi
    if ! apply_git_patch $patch ; then
      echo "Patching failed ($patch.patch)" >&2
      exit 1
    fi
  done
elif test "${GIT_PATCHLIST[0]}" != "" ; then
  echo "Git patches defined, but git patching is not enabled" >&2
  exit 1
fi

for patch in "${PATCHLIST[@]}"
do
  if test "${patch}.patch" = "$APPLY_UNTIL" ; then
    echo "$patch not applied as requested to stop"
    break
  fi
  if ! apply_patch $patch ; then
    echo "Patching failed ($patch.patch)" >&2
    exit 1
  fi
done
