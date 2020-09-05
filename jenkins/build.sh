set -e
rm -rf build
mkdir build
cd build
CC=clang CXX=clang++ cmake .. -DCMAKE_INSTALL_PREFIX=/opt/gameon_core -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja \
    -DCMAKE_CXX_FLAGS="-v" \
    -DCMAKE_C_FLAGS="-v" \
    -DCMAKE_EXE_LINKER_FLAGS="-v"
VERBOSE=1 nice ninja
