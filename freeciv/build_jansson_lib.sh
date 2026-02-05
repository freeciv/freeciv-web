# Build Jansson library for emscripten
if ! test -f "freeciv/jansson/jansson_private_config.h.in" ; then
  echo "Jansson source not found in \"${JANSSON_ROOT}\"" >&2
  exit 1
fi
cd freeciv/jansson || exit 1
export CFLAGS="-pthread"
export CXXFLAGS="-pthread"
export LDFLAGS="-pthread"
emconfigure ./configure
emmake make

cd ../.. || exit 1

