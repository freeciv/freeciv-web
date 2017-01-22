#!/bin/sh

# Freeciv server version upgrade notes
# ------------------------------------
# anek_any_action_untargeted is Freeciv bug #25401 (SVN r34748)

# Not in the upstream Freeciv server
# ----------------------------------
# 0001-Revert-Added-packet-type-web_city_info_addition-to-b reverts SVN r32928
#               freeciv_web_packets_def_changes and city_fixes need to be adjusted
#               for the fact that freeciv-web specific part of the packet_city_info
#               is in a new packet. Also receiving web-client side needs changes
# activity_null_check is for Freeciv bug #22700.
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
# freeciv_web_ruleset allows Freeciv-web to load rulesets not marked as web-compatible (civ2civ3). See discussion in patch #7514.
# webperimental_install make "make install" install webperimental.
# webgl_vision_cheat_temporary is a temporary solution to reveal terrain types to the WebGL client.
# user_changeable_gameseed allows user to change map and game seed, to get reliable benchmarks of WebGL version.

PATCHLIST="0001-Revert-Added-packet-type-web_city_info_addition-to-b freeciv_web_packets_def_changes city_fixes city_impr_fix2 city-naming-change city_fixes2 metachange text_fixes unithand-change2 current_research_cost freeciv-svn-webclient-changes init_lists_disable goto_fcweb misc_devversion_sync tutorial_ruleset savegame maphand_ch ai_traits_crash server_password barbarian-names activity_null_check message_escape freeciv_segfauls_fix scorelog_filenames disable_global_warming win_chance navajo-remove-long-city-names freeciv_web_ruleset webperimental_install anek_any_action_untargeted webgl_vision_cheat_temporary user_changeable_gameseed"

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

  for patch in $PATCHLIST
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

if ! grep "NETWORK_CAPSTRING_MANDATORY=\"$ORIGCAPSTR\"" freeciv/fc_version 2>/dev/null >/dev/null ; then
  echo "Capstring to be replaced does not match one given in version.txt" >&2
  exit 1
fi

sed "s/$ORIGCAPSTR/$WEBCAPSTR/" freeciv/fc_version > freeciv/fc_version.tmp
mv freeciv/fc_version.tmp freeciv/fc_version
chmod a+x freeciv/fc_version

for patch in $PATCHLIST
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
