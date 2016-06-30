#!/bin/sh

# Freeciv server version upgrade notes
# ------------------------------------
# CardinalityCheckOnDemand is backport of Freeciv patch #7126 (SVN r32462)
# TileExtrasSafe is backport of Freeciv patch #7127 (SVN r32475)
# NoNonnull is backport of Freeciv patch #7156 (SVN r32514)
# ScorefileWeb is Freeciv patch #7177 (SVN r32570)
# serverside_extra_assign is Freeciv patch #7178 (SVN r32585)
# ClassBonusRoadsCache is backport of Freeciv patch #7176 (SVN r32612)
# border_vision_fix is a backport of Freeciv bug #24313 (SVN r32714)
# (SVN r32928): freeciv_web_packets_def_changes and city_fixes need to be adjusted
#               for the fact that freeciv-web specific part of the packet_city_info
#               is in a new packet. Also receiving web-client side needs changes
# BuildingCrucial is backport of Freeciv bug #24526 (SVN r32955)
# MetaconnectionPersistent is Freeciv patch #7300 (SVN r32991) implementing persistent metaserver connections.
# navajo-remove-long-city-names is a quick-fix to remove city names which would be longer than MAX_LEN_NAME
#     when the name is url encoded in json protocol.
#     MAX_LEN_CITYNAME gets increased in patch #7305 (SVN r33048)
# fcweb_scorelog is Freeciv patch #7313 (SVN r33062)

# Not in the upstream Freeciv server
# ----------------------------------
# activity_null_check is for Freeciv bug #22700.
# freeciv_segfauls_fix is a workaround some segfaults in the Freeciv server. Freeciv bug #23884.
# message_escape is a patch for protecting against script injection in the message texts.
# tutorial_ruleset changes the ruleset of the tutorial to one supported by Freeciv-web.
# win_chance includes 'Chance to win' in Freeciv-web map tile popup.
# disable_global_warming is Freeciv bug #24418

PATCHLIST="freeciv_web_packets_def_changes city_fixes city_impr_fix2 city-naming-change city_fixes2 metachange text_fixes unithand-change2 current_research_cost freeciv-svn-webclient-changes init_lists_disable goto_fcweb misc_devversion_sync tutorial_ruleset savegame maphand_ch serverside_extra_assign ai_traits_crash worklists server_password barbarian-names activity_null_check add_rulesets message_escape freeciv_segfauls_fix scorelog_filenames fcweb_scorelog disable_global_warming win_chance ScorefileWeb CardinalityCheckOnDemand ClassBonusRoadsCache TileExtrasSafe NoNonnull BuildingCrucial navajo-remove-long-city-names MetaconnectionPersistent border_vision_fix"

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

# have Freeciv's "make install" add the Freeciv-web rulesets
cp -Rf data/fcweb data/webperimental freeciv/data/
