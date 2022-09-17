#!/bin/bash

set -xe
cd $(dirname $0)/..
DEPS_DIR=$(pwd)/deps

HALIDE_VERSION=14.0.0
HALIDE_URL=https://github.com/halide/Halide/releases/download/v14.0.0/Halide-14.0.0-x86-64-linux-6b9ed2afd1d6d0badf04986602c943e287d44e46.tar.gz
HALIDE_TGZ=${DEPS_DIR}/Halide-${HALIDE_VERSION}.tar.gz
HALIDE_DIR=${DEPS_DIR}/Halide

ONNXRUNTIME_VERSION=1.12.1
ONNXRUNTIME_URL=https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz
ONNXRUNTIME_TGZ=${DEPS_DIR}/onnxruntime-${ONNXRUNTIME_VERSION}.tar.gz
ONNXRUNTIME_GPU_URL=https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-gpu-${ONNXRUNTIME_VERSION}.tgz
ONNXRUNTIME_GPU_TGZ=${DEPS_DIR}/onnxruntime-gpu-${ONNXRUNTIME_VERSION}.tar.gz
ONNXRUNTIME_DIR=${DEPS_DIR}/onnxruntime
ONNXRUNTIME_GPU_DIR=${DEPS_DIR}/onnxruntime-gpu

[ -d ${DEPS_DIR} ] || mkdir ${DEPS_DIR}

[ -e ${HALIDE_TGZ} ] || curl -L -o ${HALIDE_TGZ} ${HALIDE_URL}
[ -e ${ONNXRUNTIME_TGZ} ] || curl -L -o ${ONNXRUNTIME_TGZ} ${ONNXRUNTIME_URL}
[ -e ${ONNXRUNTIME_GPU_TGZ} ] || curl -L -o ${ONNXRUNTIME_GPU_TGZ} ${ONNXRUNTIME_GPU_URL}

pushd ${DEPS_DIR}
  [ -d ${HALIDE_DIR} ] || mkdir ${HALIDE_DIR} && tar zxf ${HALIDE_TGZ} -C ${HALIDE_DIR} --strip-components 1
  [ -d ${ONNXRUNTIME_DIR} ] || mkdir ${ONNXRUNTIME_DIR} && tar zxf ${ONNXRUNTIME_TGZ} -C ${ONNXRUNTIME_DIR} --strip-components 1
  [ -d ${ONNXRUNTIME_GPU_DIR} ] || mkdir ${ONNXRUNTIME_GPU_DIR} && tar zxf ${ONNXRUNTIME_GPU_TGZ} -C ${ONNXRUNTIME_GPU_DIR} --strip-components 1
popd
