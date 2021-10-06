#!/bin/bash

set -xe

cd $(dirname $0)/..
DEPS_DIR=$(pwd)/deps

mkdir -p build
pushd build
  CC=clang-12 CXX=clang++-12 cmake .. \
    -DHalide_DIR=${DEPS_DIR}/Halide/lib/cmake/Halide \
    -DHalideHelpers_DIR=${DEPS_DIR}/Halide/lib/cmake/HalideHelpers
  cmake --build . --config RelWithDebInfo
popd
