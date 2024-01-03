#!/usr/bin/env bash

# -Synchronizes the javascript packet handler (packhand_gen.js)
#  with the definitions in packets.def.
# -extracts scenarios and helpdata from Freeciv into Freeciv-web.
# -copies sound files from Freeciv to Freeciv-web.

resolve() { echo "$(cd "$1" >/dev/null && pwd)"; }
while [[ $# -gt 0 ]]; do
  case $1 in
    -b) FCW_BASE_DIR=$(resolve "$2"); shift; shift;;
    -f) FREECIV_DIR=$(resolve "$2"); shift; shift;;
    -i) INSTALL_DIR=$(resolve "$2"); shift; shift;;
    -o) WEBAPP_DIR=$(resolve "$2"); shift; shift;;
    -d) DATA_APP_DIR=$(resolve "$2"); shift; shift;;
    *) echo "Unrecognized argument: $1"; shift;;
  esac
done
: ${FCW_BASE_DIR:?Must specify freeciv-web source root dir with -b}
: ${FREECIV_DIR:?Must specify (original) freeciv project dir with -f}
: ${INSTALL_DIR:?Must specify freeciv install dir with -i}
: ${WEBAPP_DIR:?Must specify existing freeciv-web (webapp) dir with -o}
: ${DATA_APP_DIR:?Must specify existing save-game data (webapp) dir with -d}

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

FC_DATA_DIR="${FREECIV_DIR}/data"
DOCS_DEST="${WEBAPP_DIR}/docs"
JS_DEST="${WEBAPP_DIR}/javascript"
SOUNDS_DEST="${WEBAPP_DIR}/sounds"
GAME_DEST="${DATA_APP_DIR}/savegames"

mkdir -p "${DOCS_DEST}" "${JS_DEST}" "${SOUNDS_DEST}" "${GAME_DEST}" && \
"${DIR}"/freeciv-img-extract/sync.sh -f "${FREECIV_DIR}" -o "${WEBAPP_DIR}" && \
"${DIR}"/modpack-install/modpacks_install.sh -b "${FCW_BASE_DIR}" -i "${INSTALL_DIR}" && \
"${DIR}"/helpdata_gen/ruleset_auto_gen.sh -i "${INSTALL_DIR}" -o "${WEBAPP_DIR}" && \
"${DIR}"/generate_js_hand/generate_js_hand.py -f "${FREECIV_DIR}" -o "${WEBAPP_DIR}" && \
"${DIR}"/gen_event_types/gen_event_types.py -f "${FREECIV_DIR}" -o "${WEBAPP_DIR}" && \
"${DIR}"/helpdata_gen/helpdata_gen.py -f "${FREECIV_DIR}" -o "${WEBAPP_DIR}" && \
"${DIR}"/soundspec-extract/soundspec-extract.py -f "${FREECIV_DIR}" -o "${WEBAPP_DIR}" && \
cp "${FREECIV_DIR}/data/stdsounds"/*.ogg "${SOUNDS_DEST}" && \
  echo "Copied sounds to ${SOUNDS_DEST}" && \
cp "${FREECIV_DIR}/data/scenarios"/*.sav "${GAME_DEST}" && \
  echo "Copied scenarios to ${GAME_DEST}" && \
cp "${DIR}/../LICENSE.txt" "${DOCS_DEST}" && \
(if [ -n "${UPDATE_EXTERNAL_SOURCES}" ]; then
  "${DIR}"/update-wikipedia-docs.py -f "${FREECIV_DIR}" -o "${WEBAPP_DIR}"
    echo "Wikipedia content updated."
    echo "  Some images or content may be stale - please review the generated files"
    echo "  for suitability."
else
  echo "NOT updating files generated from external sources (e.g. wikipedia)."
  echo "  Due to various risks of using external data, this is not the default behavior."
  echo "  To run these scripts, use:"
  echo "      $ > UPDATE_EXTERNAL_SOURCES=true sync-js-hand.js"
fi) && \
echo "done with sync-js-hand!"
