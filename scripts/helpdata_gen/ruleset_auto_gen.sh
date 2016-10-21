#!/bin/sh

# Freeciv has code that generates certain help texts based on the ruleset.
# This code is written in C. It is huge. Replicating it in JavaScript would
# be a huge task and probably introduce bugs. Even if someone did it it
# would be hard to keep the replicated code in sync as the corresponding
# Freeciv C code kept evolving.
#
# Freeciv has the tool freeciv-manual. It can use the ruleset based auto
# help text generation. It can output HTML. Some of its HTML output is
# machine readable enough to be usable for Freeciv-web.
#
# Generate HTML manuals for the supported rulesets.

SCRIPT_DIR="$(dirname "$0")"

cd ${SCRIPT_DIR}/../../freeciv/freeciv/ && \
./tools/freeciv-manual -r civ2civ3 && \
./tools/freeciv-manual -r classic && \
./tools/freeciv-manual -r webperimental && \
mkdir -p ../../freeciv-web/src/main/webapp/man/ && \
mv *.html ../../freeciv-web/src/main/webapp/man/
