#!/bin/bash

# Freeciv server version upgrade notes
# ------------------------------------
# 0001-Set-player-tile-owner-whenever-tile-knowledge-is-upd.patch is hrm Bug #846106
#     It was committed as 7a57cea0673432472946956d9f8c635e9f165da2
# 0025-Do-end_phase-research-updates-for-alive-players-only.patch is hrm Bug #873692
#     It was committed as 833d1fb14c3a5014bc4eeccfe652f5d9dddf3119
# 0009-Fix-cvercmp-compiler-warning-with-gcc-10-and-O3.patch is hrm Bug #886330
#     It was committed as 7527315835c16179e68080fe8f7244c76152b656
# 0010-Fix-stdinhand.c-compiler-warning-with-gcc-10-and-O3.patch is hrm Bug #886331
#     It was committed as 71eb308f799fd62920077d68d9109448d47cc7a8
# 0008-configure.ac-Drop-use-of-obsolete-AC_HEADER_TIME.patch
#     is hrm Feature #889543.
#     It was committed as 20b75101a6d7e98eef27fc49dd2304e6262cc584
# 0009-configure.ac-Drop-use-of-obsolete-AC_HEADER_STDC.patch
#     is hrm Feature #889544
#     It was committed as 54267cb9596570dad2aef2db720789d80b8c6cf7
# 0025-Don-t-hide-allied-stealth-units-on-seen-tiles.patch
#     is hrm Bug #764976
#     It was committed as 9fd4cd5397c25f99a4cba4fc27700d54bf087898
# 0005-Fix-gcc-11-stringop-overread-error-at-comparing-scen.patch
#     is hrm Bug #894423
#     It was committed as f34a882ddf9f7065acb39628fc6cb5b2cf2975dd
# 0001-Make-generated-random-seed-less-predictable.patch is hrm Bug #914184
#     It was committed as a56144bd28e2a19707cee5c5c3028a12c634d0ff

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
# longturn implements a very basic longturn mode for Freeciv-web.
# load_command_confirmation adds a log message which confirms that loading is complete, so that Freeciv-web can issue additional commands.
# pragma_pack_city_length adds pragma pack to city packet. Also sets MAX_LEN_CITYNAME 50 for large longturn games.
# endgame-mapimg is used to generate a mapimg at endgame for hall of fame.

declare -a PATCHLIST=(
  "0009-Fix-cvercmp-compiler-warning-with-gcc-10-and-O3"
  "0010-Fix-stdinhand.c-compiler-warning-with-gcc-10-and-O3"
  "0008-configure.ac-Drop-use-of-obsolete-AC_HEADER_TIME"
  "0009-configure.ac-Drop-use-of-obsolete-AC_HEADER_STDC"
  "0025-Don-t-hide-allied-stealth-units-on-seen-tiles"
  "0005-Fix-gcc-11-stringop-overread-error-at-comparing-scen"
  "0001-Make-generated-random-seed-less-predictable"
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
  "activity_null_check"
  "message_escape"
  "freeciv_segfauls_fix"
  "scorelog_filenames"
  "disable_global_warming"
  "win_chance"
  "navajo-remove-long-city-names"
  "webperimental_install"
  "longturn"
  "load_command_confirmation"
  "pragma_pack_city_length"
  "webgl_vision_cheat_temporary"
  "endgame-mapimg"
  "0001-Set-player-tile-owner-whenever-tile-knowledge-is-upd"
  "0025-Do-end_phase-research-updates-for-alive-players-only"
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
