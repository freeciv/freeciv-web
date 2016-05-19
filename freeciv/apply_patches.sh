#!/bin/sh

# activity_null_check is for Freeciv bug #22700.
# freeciv_segfauls_fix is a workaround some segfaults in the Freeciv server. Freeciv bug #23884.
# message_escape is a patch for protecting against script injection in the message texts.
# tutorial_ruleset changes the ruleset of the tutorial to one supported by Freeciv-web.
# ruleset-capability will get obsolete by Freeciv patch #6791 (SVN r31448)
# missing_trade_partner is Freeciv patch #6814 (SVN r31453)
# spaceship_post_launch is Freeciv bug #24309 (SVN r31476)
# metamessage is Freeciv patch #6876 (SVN r31682)
# 0001-JSON-specify-delta-vector-field-address is Freeciv bug #24354 (SVN r31707), included
#     here just to provide baseline that other patches apply directly to
# MapMoveCostInline is backport of Freeciv patch #6934 (SVN r31886)
# fragement is comment changes like SVN r31952, included here just to provide baseline that
#     other patches apply directly to
# Remove-redundant-canceled-orders-notification is Freeciv bug #24414 (SVN r31956)
# settings_access_level_metamessage is replaced by Freeciv patch #6967 (SVN r31962)
# AllowIextraInBetween is Freeciv bug #24395 (SVN r31970)
# GenericPacketsGen24421 is json protocol change, backported from Freeciv bug #24421 (SVN r31996)
# WarningsBase fixes compiler warnings from the code that gets properly replaced by GenericPacketsGen24421
# ArrayDimensions is json protocol change, backported from Freeciv bug #24419 (SVN r32001)
# InlineGenlistNavigation is backport of Freeciv patch #6992 (SVN r32006)
# libtoolize_no_symlinks will get obsolete by Freeciv patch #7001 (SVN r32156)
# CommentLineOfItsOwn is backport of Freeciv patch #7000 (SVN r32307)
# CardinalityCheckOnDemand is backport of Freeciv patch #7126 (SVN r32462)
# TileExtrasSafe is backport of Freeciv patch #7127 (SVN r32475)
# NoNonnull is backport of Freeciv patch #7156 (SVN r32514)
# ScorefileWeb is Freeciv patch #7177 (SVN r32570)
# serverside_extra_assign is Freeciv patch #7178 (SVN r32585)
# ClassBonusRoadsCache is backport of Freeciv patch #7176 (SVN r32612)
# win_chance includes 'Chance to win' in Freeciv-web map tile popup.
# metamessage_setting is Freeciv bug #24415
# disable_global_warming is Freeciv bug #24418

PATCHLIST="WarningsBase freeciv_web_packets_def_changes caravan_fixes1 city_fixes city_impr_fix2 city-naming-change city_fixes2 citytools_changes metachange text_fixes unithand-change2 current_research_cost freeciv-svn-webclient-changes init_lists_disable goto_fcweb misc_devversion_sync tutorial_ruleset savegame maphand_ch serverside_extra_assign libtoolize_no_symlinks ai_traits_crash ruleset-capability worklists server_password barbarian-names activity_null_check add_rulesets message_escape freeciv_segfauls_fix scorelog_filenames scorelog_set_to_client missing_trade_partner spaceship_post_launch metamessage settings_access_level_metamessage metamessage_setting disable_global_warming 0001-JSON-specify-delta-vector-field-address fragement GenericPacketsGen24421 ArrayDimensions CommentLineOfItsOwn Remove-redundant-canceled-orders-notification win_chance ScorefileWeb MapMoveCostInline AllowIextraInBetween CardinalityCheckOnDemand ClassBonusRoadsCache InlineGenlistNavigation TileExtrasSafe NoNonnull"

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
