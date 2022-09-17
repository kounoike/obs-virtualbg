#!/bin/bash

set -xe
pushd $(dirname $0)/..

OBS_VERSION=$(brew info --json=v2 --cask obs | jq -r .casks[0].version)
LLVM_VERSION=$(brew info --json=v2 llvm@13 | jq -r .formulae[0].installed[0].version)

echo "Using OBS ${OBS_VERSION}, LLVM ${LLVM_VERSION}"

[ -d deps ] || mkdir deps
if [ ! -d deps/obs-studio/build ]; then
  [ -d deps/obs-studio ] && rm -rf deps/obs-studio
  git -C deps clone --recursive --single-branch --depth 1 -b ${OBS_VERSION} https://github.com/obsproject/obs-studio.git

  pushd deps/obs-studio
    ./CI/build-macos.sh
  popd
fi


[ -d build ] && rm -rf build
mkdir build
pushd build
  # cmake .. -DobsPath=../deps/obs-studio -DLLVM_DIR=/usr/local/Cellar/llvm/12.0.1/lib/cmake/llvm
  cmake .. \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_APPLE_SILICON_PROCESSOR=x86_64 \
    -DOnnxRuntimePath=$(cd ../deps/onnxruntime; pwd) \
    -DHalide_DIR=$(cd ../deps/Halide; pwd)/lib/cmake/Halide \
    -DHalideHelpers_DIR=$(cd ../deps/Halide; pwd)/lib/cmake/HalideHelpers \
    -DLLVM_DIR=/usr/local/Cellar/llvm@13/${LLVM_VERSION}/lib/cmake/llvm \
    -DCMAKE_PREFIX_PATH=$(cd ../deps/obs-studio/build; pwd)

  cmake --build . --config Release
  cpack
popd
