#!/bin/bash

# Freeciv server version upgrade notes (backports)
# ------------------------------------------------
# osdn #????? is ticket in freeciv.org tracker:
# https://osdn.net/projects/freeciv/ticket/?????
#
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
# 0044-Meson-Include-just-stub-AI-when-server-not-built.patch
#   Included to freeciv-web for memory savings on any build
#   osdn #48193
# 0038-Give-ui_name-for-internal-actions.patch
#   Fix freeciv-manual assert failure
#   osdn #48246
# 0023-Meson-Add-dev-save-compat-support.patch
#   Make dev-format savegame compatibility to work on Meson builds
#   osdn #45610
# 0001-Fix-action_id_is_internal-act-assert-failure-on-rule.patch
#   Ruleset load time assert fix
#   osdn #48267
# 0035-Make-diplstate-struct-smaller.patch
#   Save memory with high number of players
#   osdn #48293
# 0018-Set-diplstate-max_state-correctly-for-teamed-players.patch
#   Fix failing asserts on a teamed game
#   osdn #48295
# 0026-Savecompat-Convert-SSA-Autosettlers-to-Autoworker.patch
#   Fix converting old savegames
#   osdn #48310

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
  "backports/0043-Reformat-amplio2-tiles.spec"
  "backports/0002-Meson-Compress-scenario-files-installed"
  "backports/0004-Rulesets-Drop-Clean-Pollution-and-Clean-Fallout-acti"
  "backports/0052-Correct-version-numbers-in-sg_regr-uses-to-decimal"
  "backports/0039-Protocol-Fix-sending-gives_shared_tiles"
  "backports/0044-Meson-Include-just-stub-AI-when-server-not-built"
  "backports/0038-Give-ui_name-for-internal-actions"
  "backports/0023-Meson-Add-dev-save-compat-support"
  "backports/0001-Fix-action_id_is_internal-act-assert-failure-on-rule"
  "backports/0035-Make-diplstate-struct-smaller"
  "backports/0018-Set-diplstate-max_state-correctly-for-teamed-players"
  "backports/0026-Savecompat-Convert-SSA-Autosettlers-to-Autoworker"
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
