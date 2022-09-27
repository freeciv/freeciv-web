#!/bin/bash

# Freeciv server version upgrade notes
# ------------------------------------
# osdn #????? is ticket in freeciv.org tracker:
# https://osdn.net/projects/freeciv/ticket/?????
#
# 0002-generate_packets.py-Correctly-identify-cm_parameter-
#   Fix recent regression in generating packet handling code for cma
#   osdn #45267
# 0024-Create-web-packages-only-if-there-s-web-clients-pres
#   Web-client compatibility development
#   osdn #45319
# 0023-Be-less-spammy-about-fixing-research-count-errors-fr
#   Dependency of 0034-Cancel-invalid-war-from-old-savegames
#   osdn #45344
# 0034-Cancel-invalid-war-from-old-savegames
#   Complete change to no-contact-war via alliance -rule
#   NOTE: This gets effectively reverted by a later patch,
#         but is needed for later patches to apply cleanly.
#   osdn #45345
# 0052-Add-server-side-CMA-info-to-PACKET_WEB_CITY_INFO_ADD.patch
#   Send CMA info to web-client
#   osdn #45268
# 0050-fc_strrep_resize-Fix-on-NDEBUG-builds.patch
#   Fix recent freeciv-manual regression
#   osdn #45508
# 0042-Add-inline-advance_count-wrapper-proving-that-it-ret.patch
#   Fix build with gcc-12 on high optimization level
#   osdn #45541
# 0042-Fix-map_claim_base-memory-leak.patch
#   Memory leak fix
#   osdn #45545
# 0053-Always-free-unit-build-reqs-vector-in-unit_type_free.patch
#   Make unit requirements to get properly purged before loading another ruleset
#   osdn #45572
# 0031-Protocol-Make-connection-count-UINT16-in-PACKET_CONN.patch
#   Fix potocol problems with more than 255 active connections
#   osdn #45501
# 0045-Stop-cancelling-wars-on-savegame-load.patch
#   Practically revert older patch that was causing a regression
#   osdn #45605
# 0052-Fix-how-tech-requirement-exceptions-work-wrt-barbari.patch
#   Fix to ONE issue preventing barbarians from building units
#   osdn #45632
# 0044-Add-server-support-for-web-client-to-request-CMA.patch
#   Add client -> server part of CMA protocol
#   osdn #45485
# 0004-Fix-memory-leaks-from-req_to_fstring-usage.patch
#   Memory leak fix
#   osdn #45544
# 0028-req_copy-Copy-present.patch
#   Fix to barbarian unit requirement check depending on uninialized data
#   osdn #45709
# 0038-Fix-city_add_improvement_with_gov_notice-memory-leak.patch
#   Memory leak fix
#   osdn #45707
# 0038-Fix-barbarians-exception-to-unit-tech-requirements.patch
#   Fix to final issue preventing barbarians from building units
#   osdn #45571
# 0037-Server-CMA-Try-with-default-parameters-after-cancell.patch
#   Improve server side CMA
#   osdn #45727

# Not in the upstream Freeciv server
# ----------------------------------
# tech_req-None-detection.patch
#   Fix "Tech" "None" requirements from ending to unit's requirement list.
#   Once we update past upstream 009ca51f541d8da36ca88b50145af96351350783 / hrm #846779,
#   this is to be replaced by upstream osdn #45497
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
  "0002-generate_packets.py-Correctly-identify-cm_parameter-"
  "0024-Create-web-packages-only-if-there-s-web-clients-pres"
  "0023-Be-less-spammy-about-fixing-research-count-errors-fr"
  "0034-Cancel-invalid-war-from-old-savegames"
  "0052-Add-server-side-CMA-info-to-PACKET_WEB_CITY_INFO_ADD"
  "0050-fc_strrep_resize-Fix-on-NDEBUG-builds"
  "0042-Add-inline-advance_count-wrapper-proving-that-it-ret"
  "0042-Fix-map_claim_base-memory-leak"
  "0053-Always-free-unit-build-reqs-vector-in-unit_type_free"
  "0031-Protocol-Make-connection-count-UINT16-in-PACKET_CONN"
  "0045-Stop-cancelling-wars-on-savegame-load"
  "0052-Fix-how-tech-requirement-exceptions-work-wrt-barbari"
  "0044-Add-server-support-for-web-client-to-request-CMA"
  "0004-Fix-memory-leaks-from-req_to_fstring-usage"
  "0028-req_copy-Copy-present"
  "0038-Fix-city_add_improvement_with_gov_notice-memory-leak"
  "0038-Fix-barbarians-exception-to-unit-tech-requirements"
  "0037-Server-CMA-Try-with-default-parameters-after-cancell"
  "tech_req-None-detection"
  "meson_webperimental"
  "city_impr_fix2"
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
  "barbarian-names"
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
if test "x$1" != "x" ; then
  APPLY_UNTIL="$1"
  au_found=false

  for patch in "${PATCHLIST[@]}"
  do
    if test "x$patch" = "x$APPLY_UNTIL" ; then
        au_found=true
        APPLY_UNTIL="${APPLY_UNTIL}.patch"
    elif test "x${patch}.patch" = "x$APPLY_UNTIL" ; then
        au_found=true
    fi
  done
  if test "x$au_found" != "xtrue" ; then
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
  if test "x${patch}.patch" = "x$APPLY_UNTIL" ; then
    echo "$patch not applied as requested to stop"
    break
  fi
  if ! apply_patch $patch ; then
    echo "Patching failed ($patch.patch)" >&2
    exit 1
  fi
done
