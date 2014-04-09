#!/bin/bash

# bundle and compress Freeciv-web Javascript using Google Closure Compiler
# then build Freeciv-web using Maven.

ROOTDIR="$(pwd)/.."

rm ./src/main/webapp/javascript-compressed/webclient.js -rf
kommandoen="java -jar ./src/main/webapp/WEB-INF/compiler.jar --create_source_map src/main/webapp/javascript-compressed/freeciv-web.js.map"
for f in `ls src/main/webapp/javascript/*.js`
do
  kommandoen="$kommandoen --js=$f"
done

kommandoen="$kommandoen --js_output_file=src/main/webapp/javascript-compressed/webclient.js"

# run compressor and then build using Maven
$kommandoen && printf "\n//# sourceMappingURL=freeciv-web.js.map" >> src/main/webapp/javascript-compressed/webclient.js \
&& sed -i s+src/main/webapp++g src/main/webapp/javascript-compressed/freeciv-web.js.map \
&& mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin/webapps/ROOT.war"
