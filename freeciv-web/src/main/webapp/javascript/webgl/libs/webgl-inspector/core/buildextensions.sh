#!/bin/sh
# Get the scripts absolute path
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd`
popd > /dev/null

# Make sure we're in the script folder
cd "$SCRIPTPATH"

# Install any npm modules neede
npm install

mkdir -p lib

cd dependencies
cat reset-context.css syntaxhighlighter_3.0.83/shCore.css syntaxhighlighter_3.0.83/shThemeDefault.css > ../cat.dependencies.css
cd ..

cat cat.dependencies.css ui/gli.css > lib/gli.all.css
rm cat.dependencies.css

# build lib/gli.all.js from the "build" script specified in package.json
npm run build

# Copy assets
cp -R ui/assets lib/

# Copy the lib/ directory to all the extension paths
cp -R lib/* extensions/safari/webglinspector.safariextension/
cp -R lib/* extensions/chrome/
cp -R lib/* extensions/firefox/data/

# Safari uses the chrome contentscript.js - nasty, but meh
cp extensions/chrome/contentscript.js extensions/safari/webglinspector.safariextension/

# Build Firefox Add-on.
## Sync submodules.
cd ..
git submodule sync
git submodule update --init

## Build.
cd core/extensions/firefox
make build
cd ../..
