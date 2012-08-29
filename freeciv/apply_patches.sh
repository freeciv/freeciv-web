#!/bin/sh

PATCHLIST="arctic-bugfix caravan_fixes1 city_fixes city_impr_fix2 city_name_bugfix city-naming-change citytools_changes client-fixes current_research_cost diplchanges nosave lake-fix goto_1 goto_attack1 goto_fix_1 goto_fix_2 govt-fix map-settings map_size_change metachange metapatch1 middle_click_info orders_aborted orders_invalid2 savegame-support text_fixes tileset_hack turnchange unit_dif unithand-change2 webclient-ai-attitude freeciv-svn-webclient-changes network-rewrite-1 fcnet_packets misc_devversion_sync specs_format tile_bases_roads capstr"

apply_patch() {
  patch -u -p1 -d freeciv < patches/$1.patch
}

for patch in $PATCHLIST
do
  if ! apply_patch $patch ; then
    echo "Patching failed" >&2
    exit 1
  fi
done
