#!/bin/bash

set -xe
cd $(dirname $0)/..

ORT_VERSION=1.10.0

[ -d deps/onnxruntime ] && exit 0
mkdir -p deps/onnxruntime

git -C deps clone --single-branch --depth 1 -b v${ORT_VERSION} https://github.com/microsoft/onnxruntime.git
pushd deps/onnxruntime
  ./build.sh --config RelWithDebInfo --parallel --skip_tests
  pushd build/MacOS/RelWithDebInfo
    cmake ../../../cmake -DCMAKE_INSTALL_PREFIX=$(cd ../../../..; pwd)/onnxruntime
    cmake --build . --target install
    find . -name '*.a' -exec cp {} $(cd ../../../..; pwd)/onnxruntime/lib/ \;
    # cmake --install . --prefix=$(cd ../../../..; pwd)/onnxruntime --config RelWithDebInfo
  popd
popd
