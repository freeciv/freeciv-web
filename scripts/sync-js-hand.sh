#!/bin/bash

# -Syncronizes the javascript packet handler (packhand_gen.js)
#  with the definitions in packets.def.
# -extracts scenarios and helpdata from Freeciv into Freeciv-web.
# -copies sound files from Freeciv to Freeciv-web. 

DATADIR="/var/lib/tomcat8/webapps/data/"

python generate_js_hand/generate_js_hand.py && \
cp packhand_gen.js ../freeciv-web/src/main/webapp/javascript/ && \
cp packets.js ../freeciv-web/src/main/webapp/javascript/ && \
mkdir -p ${DATADIR}savegames/ && \
mkdir -p ${DATADIR}savegames/pbem/ && \
cp ../freeciv/freeciv/data/scenarios/*.sav ${DATADIR}savegames/ && \
python3 helpdata_gen/helpdata_gen.py &&\
cp freeciv-helpdata.js ../freeciv-web/src/main/webapp/javascript/ && \
sh helpdata_gen/ruleset_auto_gen.sh && \
cp ../LICENSE.txt ../freeciv-web/src/main/webapp/docs/ &&
mkdir -p ../freeciv-web/src/main/webapp/sounds/  && \
cp ../freeciv/freeciv/data/stdsounds/*.ogg ../freeciv-web/src/main/webapp/sounds/ && \
cd soundspec-extract && python3 soundspec-extract.py && cp soundset_spec.js ../../freeciv-web/src/main/webapp/javascript/ && \
echo "done with sync-js-hand!"
