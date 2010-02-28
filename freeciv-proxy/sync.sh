# Syncronizes the javascript packet handler (packhand_gen.js)
# with the definitions in packets.def.
python generate_js_hand.py
cp packhand_gen.js ../freeciv-web/src/main/webapp/javascript/
