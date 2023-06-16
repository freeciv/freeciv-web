#!/bin/bash

# Freeciv server version upgrade notes (backports)
# ------------------------------------------------
# osdn #????? is ticket in freeciv.org tracker:
# https://osdn.net/projects/freeciv/ticket/?????
#
# 0022-Fix-out-of-bounds-on-cargo-iter.patch
#   Memory fix
#   osdn #47982
# 0021-Autoworkers-Fix-assert-failure-because-of-recursive-.patch
#   Assert failure fix
#   osdn #47992
# 0023-Savegame-Correct-loading-governor-settings.patch
#   Savegame loading fix
#   osdn #48002
# 0025-Filter-chat-messages-more-aggressively-on-freeciv-we.patch
#   Replaces former freeciv-web specific patch
#   osdn #48007
# 0029-Keep-observers-in-sync-with-city-investigation.patch
#   Protocol fix for observer connections
#   osdn #46186
# 0024-Delay-city-size-change-when-restoring-protected-unit.patch
#   Fix inconsistent data -> wrong amount of unit shield upkeep
#   osdn #48023
# 0023-Rulesave-Fix-crash-when-handling-internal-actions.patch
#   Crash fix
#   osdn #48036
# 0028-Fix-action_is_internal-crash-when-actions-are-not-se.patch
#   Crash fix
#   osdn #48009
# 0036-make_dir-Add-mode-parameter.patch
#   Improvement to rebase freeciv-web's own savegame.patch on
#   osdn #48094
# 0039-Meson-Make-it-possible-to-disable-server-build.patch
#   Reworked server build option
#   osdn #48098
# 0017-Meson-Don-t-try-to-link-against-zlib-on-emscripten-b.patch
#   Dependency of 0028-Meson-Fix-gzipped-saves-support.patch
#   osdn #48043
# 0028-Meson-Fix-gzipped-saves-support.patch
#   Fix handling of compressed saves
#   osdn #48101
# 0037-Add-ACTIVITY_CLEAN-to-tile-changing-activities.patch
#   Fix to ACTIVITY_CLEAN support
#   osdn #48147
# 0043-Reformat-amplio2-tiles.spec.patch
#   Work around freeciv-web auto_worker icon problem
#   osdn #48179
# 0002-Meson-Compress-scenario-files-installed.patch
#   Fix an autotools -> meson regression of not compressing scenarios
#   osdn #47826
# 0004-Rulesets-Drop-Clean-Pollution-and-Clean-Fallout-acti.patch
#   Use new generic "Clean" action only in rulesets
#   osdn #47628
# 0052-Correct-version-numbers-in-sg_regr-uses-to-decimal.patch
#   Correction to savegame loading error reporting
#   osdn #48212
# 0039-Protocol-Fix-sending-gives_shared_tiles.patch
#   Network protocol fix
#   osdn #48119

# Not in the upstream Freeciv server
# ----------------------------------
# meson_webperimental installs webperimental ruleset
# freeciv_segfauls_fix is a workaround some segfaults in the Freeciv server. Freeciv bug #23884.
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
  "backports/0022-Fix-out-of-bounds-on-cargo-iter"
  "backports/0021-Autoworkers-Fix-assert-failure-because-of-recursive-"
  "backports/0023-Savegame-Correct-loading-governor-settings"
  "backports/0025-Filter-chat-messages-more-aggressively-on-freeciv-we"
  "backports/0029-Keep-observers-in-sync-with-city-investigation"
  "backports/0024-Delay-city-size-change-when-restoring-protected-unit"
  "backports/0023-Rulesave-Fix-crash-when-handling-internal-actions"
  "backports/0028-Fix-action_is_internal-crash-when-actions-are-not-se"
  "backports/0036-make_dir-Add-mode-parameter"
  "backports/0039-Meson-Make-it-possible-to-disable-server-build"
  "backports/0017-Meson-Don-t-try-to-link-against-zlib-on-emscripten-b"
  "backports/0028-Meson-Fix-gzipped-saves-support"
  "backports/0037-Add-ACTIVITY_CLEAN-to-tile-changing-activities"
  "backports/0043-Reformat-amplio2-tiles.spec"
  "backports/0002-Meson-Compress-scenario-files-installed"
  "backports/0004-Rulesets-Drop-Clean-Pollution-and-Clean-Fallout-acti"
  "backports/0052-Correct-version-numbers-in-sg_regr-uses-to-decimal"
  "backports/0039-Protocol-Fix-sending-gives_shared_tiles"
  "meson_webperimental"
  "metachange"
  "text_fixes"
  "freeciv-svn-webclient-changes"
  "goto_fcweb"
  "tutorial_ruleset"
  "savegame"
  "maphand_ch"
  "server_password"
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
