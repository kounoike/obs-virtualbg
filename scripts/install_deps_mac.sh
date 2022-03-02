#!/bin/bash

set -xeu
cd $(dirname $0)/..
DEPS_DIR=$(pwd)/deps

HALIDE_VERSION=13.0.4
HALIDE_X86_URL=https://github.com/halide/Halide/releases/download/v13.0.4/Halide-13.0.4-x86-64-osx-3a92a3f95b86b7babeed7403a330334758e1d644.tar.gz
HALIDE_X86_TGZ=${DEPS_DIR}/Halide-x86_64-${HALIDE_VERSION}.tar.gz
HALIDE_X86_DIR=${DEPS_DIR}/Halide-x86_64

HALIDE_ARM_URL=https://github.com/halide/Halide/releases/download/v13.0.4/Halide-13.0.4-arm-64-osx-3a92a3f95b86b7babeed7403a330334758e1d644.tar.gz
HALIDE_ARM_TGZ=${DEPS_DIR}/Halide-arm64-${HALIDE_VERSION}.tar.gz
HALIDE_ARM_DIR=${DEPS_DIR}/Halide-arm64

ONNXRUNTIME_VERSION=1.10.0
ONNXRUNTIME_URL=https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-osx-universal2-${ONNXRUNTIME_VERSION}.tgz
ONNXRUNTIME_TGZ=${DEPS_DIR}/onnxruntime-osx-universal2-${ONNXRUNTIME_VERSION}.tgz
ONNXRUNTIME_DIR=${DEPS_DIR}/onnxruntime

[ -d ${DEPS_DIR} ] || mkdir ${DEPS_DIR}
[ -e ${HALIDE_X86_TGZ} ] || curl -o ${HALIDE_X86_TGZ} -L ${HALIDE_X86_URL}
[ -d ${HALIDE_X86_DIR} ] || mkdir ${HALIDE_X86_DIR} && tar zxf ${HALIDE_X86_TGZ} -C ${HALIDE_X86_DIR} --strip-components 1

[ -e ${HALIDE_ARM_TGZ} ] || curl -o ${HALIDE_ARM_TGZ} -L ${HALIDE_ARM_URL}
[ -d ${HALIDE_ARM_DIR} ] || mkdir ${HALIDE_ARM_DIR} && tar zxf ${HALIDE_ARM_TGZ} -C ${HALIDE_ARM_DIR} --strip-components 1

[ -e ${ONNXRUNTIME_TGZ} ] || curl -o ${ONNXRUNTIME_TGZ} -L ${ONNXRUNTIME_URL}
[ -d ${ONNXRUNTIME_DIR} ] || mkdir ${ONNXRUNTIME_DIR} && tar zxf ${ONNXRUNTIME_TGZ} -C ${ONNXRUNTIME_DIR} --strip-components 1

# for update obs version.
brew update

# brew install onnxruntime
brew install llvm@12
brew pin llvm
brew install obs --cask

