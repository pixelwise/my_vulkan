set -e
rm -rf build
mkdir build
cd build
CC=clang CXX=clang++ cmake .. -DCMAKE_INSTALL_PREFIX=/opt/gameon_core -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja
VERBOSE=1 nice ninja -k10000
