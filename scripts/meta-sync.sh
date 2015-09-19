wget -q -O ../resin/webapps/ROOT/meta/freeciv_org_metaserver.tmp http://meta.freeciv.org/
sed -i.bak s,/index.php,http://meta.freeciv.org/index.php,g ../resin/webapps/ROOT/meta/freeciv_org_metaserver.tmp
tail -n+41 ../resin/webapps/ROOT/meta/freeciv_org_metaserver.tmp > ../resin/webapps/ROOT/meta/freeciv_org_metaserver.html
