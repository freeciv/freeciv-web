# Syncronizes the javascript packet handler (packhand_gen.js)
# with the definitions in packets.def.
python generate_js_hand/generate_js_hand.py && \
cp packhand_gen.js ../freeciv-web/src/main/webapp/javascript/ && \
cp packets.js ../freeciv-web/src/main/webapp/javascript/ && \
mkdir -p ../resin/webapps/data/savegames/ && \
cp ../freeciv/freeciv/data/scenarios/*.sav ../resin/webapps/data/savegames/ && \
python3.5 helpdata_gen/helpdata_gen.py &&\
cp freeciv-helpdata.js ../freeciv-web/src/main/webapp/javascript/ && \
cp ../LICENSE.txt ../freeciv-web/src/main/webapp/docs/ &&
echo "done with sync-js-hand!"
