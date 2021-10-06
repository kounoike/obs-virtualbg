#!/bin/bash

apt-get install -y libobs-dev
apt-get install python3-pip
pip3 install cmake

DEPS_DIR=$(cd $(dirname $0)/..;pwd)/deps
[ -d ${DEPS_DIR} ] || mkdir ${DEPS_DIR}

curl -L -o ${DEPS_DIR}/llvm.sh https://apt.llvm.org/llvm.sh
chmod +x ${DEPS_DIR}/llvm.sh
${DEPS_DIR}/llvm.sh 12
