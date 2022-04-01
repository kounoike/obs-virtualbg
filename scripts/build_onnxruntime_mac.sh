#!/bin/bash

set -xe
cd $(dirname $0)/..

ORT_VERSION=1.11.0

[ -d deps/onnxruntime ] && exit 0
mkdir -p deps/onnxruntime

# git -C deps clone --single-branch --depth 1 -b v${ORT_VERSION} https://github.com/microsoft/onnxruntime.git
git -C deps clone --single-branch --depth 1 -b master https://github.com/microsoft/onnxruntime.git
pushd deps/onnxruntime
  ./build.sh --config RelWithDebInfo --parallel \
    --use_coreml \
    --skip_tests \
    --cmake_extra_defines=onnxruntime_BUILD_UNIT_TESTS=OFF \
    --osx_arch=x86_64 \
    --cmake_extra_defines=CMAKE_APPLE_SILICON_PROCESSOR=x86_64 \
    --cmake_extra_defines=CMAKE_OSX_DEPLOYMENT_TARGET=11.0
  pushd build/MacOS/RelWithDebInfo
    cmake ../../../cmake -DCMAKE_INSTALL_PREFIX=$(cd ../../../..; pwd)/onnxruntime
    cmake --build . --target install
    find . -name '*.a' -exec cp {} $(cd ../../../..; pwd)/onnxruntime/lib/ \;
    # cmake --install . --prefix=$(cd ../../../..; pwd)/onnxruntime --config RelWithDebInfo
  popd
  cp -f include/onnxruntime/core/session/*.h include/onnxruntime/core/providers/coreml/*.h include/
popd
