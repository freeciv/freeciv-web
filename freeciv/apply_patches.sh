#!/bin/bash

# Freeciv server version upgrade notes (backports)
# ------------------------------------------------
# osdn #????? is ticket in freeciv.org tracker:
# https://osdn.net/projects/freeciv/ticket/?????
#
# 0038-Fix-city_add_improvement_with_gov_notice-memory-leak.patch
#   Memory leak fix
#   osdn #45707
# 0038-Fix-barbarians-exception-to-unit-tech-requirements.patch
#   Fix to final issue preventing barbarians from building units
#   osdn #45571
# 0037-Server-CMA-Try-with-default-parameters-after-cancell.patch
#   Improve server side CMA
#   osdn #45727
# 0033-Add-info-about-what-can-be-built-to-city-web-additio.patch
#   Improve web-client protocol
#   osdn #45747
# 0023-Meson-Define-HAVE_VSNPRINTF-and-HAVE_WORKING_VSNPRIN.patch
#   Make the server use proper vsnprintf() instead of the fallback implementation
#   Important mainly because of issues in the fallback implementation
#   (fixes to those issues not backported to freeciv-web as this makes them redundant)
#   osdn #45706
# 0018-Stop-memory-corruption-on-dai_data_phase_begin.patch
#   Fix a memory corruption issue
#   osdn #45768
# 0016-Make-vision-site-able-to-hold-maximum-city-name-leng
#   Fix long city names of FoW map
#   osdn #45787
# 0040-Set-barbarians-as-AI-before-initializing-them-otherw
#   Support one freeciv-web modification, removing need for another
#   freeciv-modification. Fixes handling of animals barbarian
#   player that wasn't handled by freeciv-web at all.
#   osdn #45808
# 0005-Macrofy-city_owner
#   Textual dependency to 0036-Make-city-name-allocation-dynamic.patch
#   Optimization
#   #45634
# 0036-Make-city-name-allocation-dynamic.patch
#   Do not reserve memory for every city name by the longest possible.
#   This patch handles only the main map city names - player/FoW maps
#   still use static city name space.
#   osdn #45786
# 0053-Add-MAX_CITY_TILES-to-fc_types.h.patch
#   Make MAX_CITY_TILES definition available for the network protocol
#   osdn #45886
# 0047-load_rulesetdir-Destroy-actionfile.patch
#   Memory leak fix
#   osdn #45909

# Not in the upstream Freeciv server
# ----------------------------------
# meson_webperimental installs webperimental ruleset
# freeciv_segfauls_fix is a workaround some segfaults in the Freeciv server. Freeciv bug #23884.
# message_escape is a patch for protecting against script injection in the message texts.
# tutorial_ruleset changes the ruleset of the tutorial to one supported by Freeciv-web.
#      - This should be replaced by modification of the tutorial scenario that allows it to
#        work with multiple rulesets (Requires patch #7362 / SVN r33159)
# win_chance includes 'Chance to win' in Freeciv-web map tile popup.
# disable_global_warming is Freeciv bug #24418
# navajo-remove-long-city-names is a quick-fix to remove city names which would be longer than MAX_LEN_NAME
#     when the name is url encoded in json protocol.
#     MAX_LEN_CITYNAME was increased in patch #7305 (SVN r33048)
#     Giving one of the longer removed city names to a new city still causes problems.
# webgl_vision_cheat_temporary is a temporary solution to reveal terrain types to the WebGL client.
# longturn implements a very basic longturn mode for Freeciv-web.
# load_command_confirmation adds a log message which confirms that loading is complete, so that Freeciv-web can issue additional commands.
# cityname_length reduces MAX_LEN_CITYNAME to 50 for large longturn games.
# endgame-mapimg is used to generate a mapimg at endgame for hall of fame.

declare -a PATCHLIST=(
  "0038-Fix-city_add_improvement_with_gov_notice-memory-leak"
  "0038-Fix-barbarians-exception-to-unit-tech-requirements"
  "0037-Server-CMA-Try-with-default-parameters-after-cancell"
  "0033-Add-info-about-what-can-be-built-to-city-web-additio"
  "0023-Meson-Define-HAVE_VSNPRINTF-and-HAVE_WORKING_VSNPRIN"
  "0018-Stop-memory-corruption-on-dai_data_phase_begin"
  "0016-Make-vision-site-able-to-hold-maximum-city-name-leng"
  "0040-Set-barbarians-as-AI-before-initializing-them-otherw"
  "backports/0005-Macrofy-city_owner"
  "backports/0036-Make-city-name-allocation-dynamic"
  "backports/0053-Add-MAX_CITY_TILES-to-fc_types.h"
  "backports/0047-load_rulesetdir-Destroy-actionfile"
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
  "disable_global_warming"
  "win_chance"
  "navajo-remove-long-city-names"
  "longturn"
  "load_command_confirmation"
  "cityname_length"
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
