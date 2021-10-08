#!/bin/bash

set -xeu
cd $(dirname $0)/..
DEPS_DIR=$(pwd)/deps

HALIDE_VERSION=12.0.1
HALIDE_URL=https://github.com/halide/Halide/releases/download/v12.0.1/Halide-12.0.1-x86-64-osx-5dabcaa9effca1067f907f6c8ea212f3d2b1d99a.tar.gz
HALIDE_TGZ=${DEPS_DIR}/Halide-${HALIDE_VERSION}.tar.gz
HALIDE_DIR=${DEPS_DIR}/Halide

[ -d ${DEPS_DIR} ] || mkdir ${DEPS_DIR}
[ -e ${HALIDE_TGZ} ] || curl -o ${HALIDE_TGZ} -L ${HALIDE_URL}
[ -d ${HALIDE_DIR} ] || mkdir ${HALIDE_DIR} && tar zxf ${HALIDE_TGZ} -C ${HALIDE_DIR} --strip-components 1

# for update obs version.
brew update

# brew install onnxruntime
brew install llvm@12
brew install obs --cask


