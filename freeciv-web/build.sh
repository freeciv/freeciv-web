
ROOTDIR="$(pwd)/../.."

cd src/main/webapp/ &&
sh compress-js.sh &&
cd ../../../ &&
mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin/webapps/ROOT.war" && cd "${ROOTDIR}/resin/webapps/" && rm -rf ROOT && unzip ROOT.war -d ROOT
