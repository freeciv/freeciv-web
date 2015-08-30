#!/bin/sh

# activity_null_check is for Freeciv bug #22700.
# city_traderoute_hotfix is for Freeciv bug #23708.
# city_occupied_fix is Freeciv bug #23813. Committed in Freeciv SVN r29682
# min_curl is Freeciv bug #23824. Committed in Freeciv SVN r29702
# estring is Freeciv patch #6295. Committed in Freeciv SVN r29703
# estring-city-name is Freeciv patch #6296. Committed in Freeciv SVN r29704

PATCHLIST="min_curl estring estring-city-name freeciv_web_all_packets_def_changes caravan_fixes1 city_fixes city_impr_fix2 city-naming-change city_fixes2 citytools_changes map-settings metachange text_fixes unithand-change2 webclient-ai-attitude current_research_cost freeciv-svn-webclient-changes network-rewrite-1 fcnet_packets misc_devversion_sync scenario_ruleset savegame maphand_ch serverside_extra_assign libtoolize_no_symlinks spacerace city_disbandable ai_traits_crash unittools ruleset-capability worklists server_password aifill barbarian-names activity_null_check add_rulesets NoDeltaHeader city_traderoute_hotfix city_occupied_fix savegame_comp_segfault"

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
  if test "x$patch" = "x$APPLY_UNTIL" ; then
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
