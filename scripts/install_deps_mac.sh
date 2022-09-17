#!/bin/bash

set -xeu
cd $(dirname $0)/..
DEPS_DIR=$(pwd)/deps

HALIDE_VERSION=14.0.0
HALIDE_URL=https://github.com/halide/Halide/releases/download/v14.0.0/Halide-14.0.0-x86-64-osx-6b9ed2afd1d6d0badf04986602c943e287d44e46.tar.gz
HALIDE_TGZ=${DEPS_DIR}/Halide-${HALIDE_VERSION}.tar.gz
HALIDE_DIR=${DEPS_DIR}/Halide

[ -d ${DEPS_DIR} ] || mkdir ${DEPS_DIR}
[ -e ${HALIDE_TGZ} ] || curl -o ${HALIDE_TGZ} -L ${HALIDE_URL}
[ -d ${HALIDE_DIR} ] || mkdir ${HALIDE_DIR} && tar zxf ${HALIDE_TGZ} -C ${HALIDE_DIR} --strip-components 1

# for update obs version.
brew update

# brew install onnxruntime
brew install llvm@13
brew pin llvm
brew install obs --cask

