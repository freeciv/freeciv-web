#!/bin/sh

# Freeciv server version upgrade notes
# ------------------------------------

# Not in the upstream Freeciv server
# ----------------------------------
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
# webperimental_install make "make install" install webperimental.
# webgl_vision_cheat_temporary is a temporary solution to reveal terrain types to the WebGL client.
# user_changeable_gameseed allows user to change map and game seed, to get reliable benchmarks of WebGL version.
# longturn implements a very basic longturn mode for Freeciv-web.
# multiplayer_movement_rates is http://www.hostedredmine.com/issues/654404
# max_map_size increases MAP_MAX_SIZE.
# load_command_confirmation adds a log message which confirms that loading is complete, so that Freeciv-web can issue additional commands.
# pragma_pack_city_length adds pragma pack to city packet. Also sets MAX_LEN_CITYNAME 50 for large longturn games.
# endgame-mapimg is used to generate a mapimg at endgame for hall of fame.
# mapimg_bugfix is http://www.hostedredmine.com/issues/707912.

PATCHLIST="freeciv_web_packets_def_changes city_impr_fix2 city-naming-change city_fixes2 metachange text_fixes unithand-change2 current_research_cost freeciv-svn-webclient-changes init_lists_disable goto_fcweb misc_devversion_sync tutorial_ruleset savegame maphand_ch ai_traits_crash server_password barbarian-names activity_null_check message_escape freeciv_segfauls_fix scorelog_filenames disable_global_warming win_chance navajo-remove-long-city-names webperimental_install user_changeable_gameseed multiplayer_movement_rates longturn max_map_size load_command_confirmation pragma_pack_city_length webgl_vision_cheat_temporary endgame-mapimg mapimg_bugfix "

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
