
RESINVER="4.0.36"

ROOTDIR="$(pwd)/../.."

cd src/main/webapp/ &&
sh compress-js.sh &&
cd ../../../ &&
mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin-${RESINVER}/webapps/ROOT.war" && cd "${ROOTDIR}/resin-${RESINVER}/webapps/" && rm -rf ROOT && unzip ROOT.war -d ROOT
