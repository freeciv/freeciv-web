#!/bin/sh

PATCHLIST="caravan_fixes1 city_fixes city_impr_fix2 city_name_bugfix city-naming-change city_fixes2 citytools_changes client-fixes current_research_cost diplchanges goto_1 goto_attack1 goto_fix_1 goto_fix_2 govt-fix map-settings map_size_change metachange metatype packets middle_click_info orders_aborted orders_invalid2 text_fixes tileset_hack turnchange unithand-change2 webclient-ai-attitude freeciv-svn-webclient-changes network-rewrite-1 fcnet_packets misc_devversion_sync specs_format scenario_ruleset tile_bases_roads savegame savegame2 maphand_ch observer_fix"

apply_patch() {
  patch -u -p1 -d freeciv < patches/$1.patch
}

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
  if ! apply_patch $patch ; then
    echo "Patching failed ($patch.patch)" >&2
    exit 1
  fi
done
