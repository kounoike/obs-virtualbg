#!/bin/bash

set -xe

cd $(dirname $0)/..
DEPS_DIR=$(pwd)/deps

mkdir -p build
pushd build
  cmake .. \
    -DHalide_DIR=${DEPS_DIR}/Halide/lib/cmake/Halide \
    -DHalideHelpers_DIR=${DEPS_DIR}/Halide/lib/cmake/HalideHelpers \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DOBS_PLUGIN_DESTINATION='/usr/lib/obs-plugins' \
    -DCMAKE_INSTALL_RPATH="/usr/lib/obs-virtualbg"
  cmake --build . --config RelWithDebInfo
  cpack
popd
