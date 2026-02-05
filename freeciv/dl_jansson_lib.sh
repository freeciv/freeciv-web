#!/usr/bin/env -S bash -e
# Downloads and places the Jansson library source code in jansson/
# Remove old version
rm -Rf jansson
# Download Jansson source code
wget https://github.com/akheron/jansson/releases/download/v2.15.0/jansson-2.15.0.tar.gz
tar -xzf jansson-2.15.0.tar.gz
mv jansson-2.15.0 freeciv/jansson
rm jansson-2.15.0.tar.gz
