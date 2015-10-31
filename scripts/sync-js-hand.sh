# Syncronizes the javascript packet handler (packhand_gen.js)
# with the definitions in packets.def.
python generate_js_hand/generate_js_hand.py && \
cp packhand_gen.js ../freeciv-web/src/main/webapp/javascript/ && \
cp packets.js ../freeciv-web/src/main/webapp/javascript/ && \
cp ../freeciv/freeciv/data/scenarios/*.sav ../freeciv-web/src/main/webapp/savegames/ && \
cp ../freeciv/data/europe_1901.sav ../freeciv-web/src/main/webapp/savegames/ && \
python3.4 helpdata_gen/helpdata_gen.py &&\
cp freeciv-helpdata.js ../freeciv-web/src/main/webapp/javascript/ && \
cp ../LICENSE.txt ../freeciv-web/src/main/webapp/docs/ &&
echo "done with sync-js-hand!"
