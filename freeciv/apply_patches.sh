#!/bin/bash

# Freeciv server version upgrade notes (backports)
# ------------------------------------------------
# osdn #????? is ticket in freeciv.org tracker:
# https://osdn.net/projects/freeciv/ticket/?????
#
# 0007-Move-PACKET_CITY_RALLY_POINT-unpacking-to-common.patch
#   Dependency of 0018-Send-rally-point-separately-from-PACKET_CITY_INFO
#   osdn #46101
# 0018-Send-rally-point-separately-from-PACKET_CITY_INFO.patch
#   Protocol update
#   osdn #46080
# 0034-Stop-registering-hard-requirement-that-has-no-users.patch
#   Memory leak fix
#   osdn #45910
# 0034-Increase-MAX_LEN_CITYNAME-to-120.patch
#   Support longer citynames
#   osdn #46096
# 0036-Path-finding-Make-pf_fuel_pos-cost-an-int.patch
#   Fix an move cost overflow in path finding
#   osdn #46136
# 0050-AI-Check-that-hunt-target-is-targetable.patch
#   AI hunter crash fix
#   osdn #46176
# 0048-Disable-server-side-CMA-when-overridden.patch
#   Improve behavior of the server side CMA
#   osdn #46213
# 0050-Fix-assert-preventing-setting-Restricted-AI-level.patch
#   Fix failure to set AI level "Restricted"
#   osdn #46214

# Not in the upstream Freeciv server
# ----------------------------------
# meson_webperimental installs webperimental ruleset
# freeciv_segfauls_fix is a workaround some segfaults in the Freeciv server. Freeciv bug #23884.
# message_escape is a patch for protecting against script injection in the message texts.
# tutorial_ruleset changes the ruleset of the tutorial to one supported by Freeciv-web.
#      - This should be replaced by modification of the tutorial scenario that allows it to
#        work with multiple rulesets (Requires patch #7362 / SVN r33159)
# win_chance includes 'Chance to win' in Freeciv-web map tile popup.
# webgl_vision_cheat_temporary is a temporary solution to reveal terrain types to the WebGL client.
# longturn implements a very basic longturn mode for Freeciv-web.
# load_command_confirmation adds a log message which confirms that loading is complete, so that Freeciv-web can issue additional commands.
# endgame-mapimg is used to generate a mapimg at endgame for hall of fame.

declare -a PATCHLIST=(
  "backports/0007-Move-PACKET_CITY_RALLY_POINT-unpacking-to-common"
  "backports/0018-Send-rally-point-separately-from-PACKET_CITY_INFO"
  "backports/0034-Stop-registering-hard-requirement-that-has-no-users"
  "backports/0034-Increase-MAX_LEN_CITYNAME-to-120"
  "backports/0036-Path-finding-Make-pf_fuel_pos-cost-an-int"
  "backports/0050-AI-Check-that-hunt-target-is-targetable"
  "backports/0048-Disable-server-side-CMA-when-overridden"
  "backports/0050-Fix-assert-preventing-setting-Restricted-AI-level"
  "meson_webperimental"
  "city-naming-change"
  "metachange"
  "text_fixes"
  "freeciv-svn-webclient-changes"
  "goto_fcweb"
  "misc_devversion_sync"
  "tutorial_ruleset"
  "savegame"
  "maphand_ch"
  "ai_traits_crash"
  "server_password"
  "message_escape"
  "freeciv_segfauls_fix"
  "scorelog_filenames"
  "win_chance"
  "longturn"
  "load_command_confirmation"
  "webgl_vision_cheat_temporary"
  "endgame-mapimg"
)

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

  for patch in "${PATCHLIST[@]}"
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
