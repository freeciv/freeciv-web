#!/bin/bash

# Freeciv server version upgrade notes (backports)
# ------------------------------------------------
# osdn #????? is ticket in freeciv.org tracker:
# https://osdn.net/projects/freeciv/ticket/?????
#
# 0031-Lua-Always-pass-lua_Integer-to-API_TYPE_INT.patch
#   Lua API fix
#   osdn #48722
# 0036-tile_move_cost_ptrs-Make-cardinal_move-signed.patch
#   Unit movemenet handling fix
#   osdn #48737
# 0047-Meson-Turn-audio-option-to-a-combo.patch
#   Rework disabling audio
#   osdn #48757
# 0008-Add-new-bitvector-utility-functions.patch
#   New bitvector utility functions
#   osdn #48731
# 0047-Add-bv_match_dbv-utility-function.patch
#   New bitvector utility function
#   osdn #48771
# 0057-Fix-bitvector-copy-functions.patch
#   Fix to bitvector utility functions
#   osdn #48772
# 0064-featured_text.-ch-Replace-NULL-with-nullptr.patch
#   Update baseline of a freeciv-web patch
#   osdn #48793
# 0050-Cache-territory-claiming-base-types.patch
#   Performance improvement
#   osdn #47345
# 0050-Correct-filling-of-territory-claiming-extras-cache.patch
#   Fix terr claiming extras cache
#   osdn #48838
# 0025-Autoworkers-Use-ADV_WANTS_EQ-for-comparing-tile-valu.patch
#   Autoworkers work value fix
#   osdn #48842
# 0032-mapimg_help-Fix-format-overflow-warning.patch
#   gcc-14 warning fix
#   osdn #48850
# 0031-city_from_wonder-Fix-illegal-array-subscript-warning.patch
#   gcc-14 warning fix
#   osdn #48849
# 0029-Fix-dbv_copy.patch
#   Memory handling fix
#   osdn #48869

# Not in the upstream Freeciv server
# ----------------------------------
# meson_webperimental installs webperimental ruleset
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
  "backports/0031-Lua-Always-pass-lua_Integer-to-API_TYPE_INT"
  "backports/0036-tile_move_cost_ptrs-Make-cardinal_move-signed"
  "backports/0047-Meson-Turn-audio-option-to-a-combo"
  "backports/0008-Add-new-bitvector-utility-functions"
  "backports/0047-Add-bv_match_dbv-utility-function"
  "backports/0057-Fix-bitvector-copy-functions"
  "backports/0064-featured_text.-ch-Replace-NULL-with-nullptr"
  "backports/0050-Cache-territory-claiming-base-types"
  "backports/0050-Correct-filling-of-territory-claiming-extras-cache"
  "backports/0025-Autoworkers-Use-ADV_WANTS_EQ-for-comparing-tile-valu"
  "backports/0032-mapimg_help-Fix-format-overflow-warning"
  "backports/0031-city_from_wonder-Fix-illegal-array-subscript-warning"
  "backports/0029-Fix-dbv_copy"
  "meson_webperimental"
  "metachange"
  "text_fixes"
  "freeciv-svn-webclient-changes"
  "goto_fcweb"
  "tutorial_ruleset"
  "savegame"
  "maphand_ch"
  "server_password"
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
