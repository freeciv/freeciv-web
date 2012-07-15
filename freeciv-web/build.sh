
ROOTDIR="$(pwd)/../.."

cd src/main/webapp/ &&
sh compress-js.sh &&
cd ../../../ &&
mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin-4.0.29/webapps/ROOT.war" && cd "${ROOTDIR}/resin-4.0.29/webapps/" && rm -rf ROOT && unzip ROOT.war -d ROOT
