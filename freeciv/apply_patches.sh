#!/bin/bash

# Freeciv server version upgrade notes
# ------------------------------------
# 0001-Fix-segfault-at-loading-older-format-savegame is hrm Bug #840623
#     It was comitted in ab1c8f2a914300783f769819bed37c848c66721a
# max_map_size is hrm Feature #847376
#     It was comitted as f2173409a8e09fea9fecfb79ddf7dcf64fdab425
# F3 is hrm Feature #868944
#     It was committed as b1c1846a346a873f3690aa171123b493629a6c02
# 0003-Switch-from-python-to-python3.patch is hrm Feature #842994
#     It was committed as b6e01c4068e364d9f375971d8f4de9a6d7776f47
# 0015-savegame3.c-Remove-aifill-players-after-rulesets-loa is hrm Bug #850666
#     It was committed as 715025fb957d3a0f2c33dfece45e6d6f783f795b
# 0001-Terminate-format-escapes-list.patch is hrm Bug #851727
#     It was committed as 17938c639f239381c0d1fcfcce356b49f5e86100
# 0001-Refactor-code-to-avoid-gcc-10-warning.patch is hrm Bug #851728
#     It was committed as f2da7a48499b1f5bd7bcccd8fdb33814521da1f2
# 0023-Fix-sell_random_unit-crash-with-recursive-transports.patch is hrm Bug #852938
#     It was committed as cb2be38ff30d899e9eced77c8e918d9f879c1e59
# 0005-Fix-ghost-unit-issue-when-unit-is-loaded-to-an-trans.patch is hrm Bug #858214
#     It was committed as f32a2bae0e88f665021b579af12c346b20e86c6d
# 0001-Fix-clang-9-warnings.patch is hrm Bug #859248
#     It was committed as 5fc3a22b3a1681d80c68b9ec0b8f0cf0b78ca239
# 0001-Fix-division-by-zero-when-transforming-unit-with-zer.patch is hrm Bug #868905
#     It was committed as d9c010701fda075d083aea600adade73cef45020
# 0006-debug.m4-Set-always-active-compiler-flags-last-not-f.patch is hrm Bug #868533
#     It was committed as 72b2b9839aeaff5178470fc9e77926cfeba2dede
# 0024-mapimg_colortest-Fix-compiler-warning-with-O3.patch is hrm Bug #870481
#     It was committed as 53d2ac716d188ba068f51b800ebf52f13512b9a7
# 0001-Set-player-tile-owner-whenever-tile-knowledge-is-upd.patch is hrm Bug #846106
#     It was committed as 7a57cea0673432472946956d9f8c635e9f165da2
# 0025-Do-end_phase-research-updates-for-alive-players-only.patch is hrm Bug #873692
#     It was committed as 833d1fb14c3a5014bc4eeccfe652f5d9dddf3119

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
  "freeciv_web_packets_def_changes"
  "city_impr_fix2"
  "city-naming-change"
  "metachange"
  "text_fixes"
  "F3"
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
  "max_map_size"
  "load_command_confirmation"
  "pragma_pack_city_length"
  "webgl_vision_cheat_temporary"
  "endgame-mapimg"
  "0001-Fix-segfault-at-loading-older-format-savegame"
  "0003-Switch-from-python-to-python3"
  "0015-savegame3.c-Remove-aifill-players-after-rulesets-loa"
  "0001-Terminate-format-escapes-list"
  "0001-Refactor-code-to-avoid-gcc-10-warning"
  "0023-Fix-sell_random_unit-crash-with-recursive-transports"
  "0005-Fix-ghost-unit-issue-when-unit-is-loaded-to-an-trans"
  "0001-Fix-clang-9-warnings"
  "0001-Fix-division-by-zero-when-transforming-unit-with-zer"
  "0006-debug.m4-Set-always-active-compiler-flags-last-not-f"
  "0024-mapimg_colortest-Fix-compiler-warning-with-O3"
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
