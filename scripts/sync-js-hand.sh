# Syncronizes the javascript packet handler (packhand_gen.js)
# with the definitions in packets.def.
python generate_js_hand/generate_js_hand.py && \
cp packhand_gen.js ../freeciv-web/src/main/webapp/javascript/ && \
cp packets.js ../freeciv-web/src/main/webapp/javascript/ && \
cp ../freeciv/scenarios/*.sav ../freeciv-web/src/main/webapp/savegames/ && \
cp ../freeciv/freeciv/data/scenarios/tutorial.sav ../freeciv-web/src/main/webapp/savegames/ && \
echo "done!"
