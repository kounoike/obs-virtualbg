#!/bin/bash

apt-get install -y libobs-dev
apt-get install python3-pip
pip3 install cmake

DEPS_DIR=$(dirname $0)/../deps

curl -L -o ${DEPS_DIR}/llvm.sh https://apt.llvm.org/llvm.sh
chmod +x ${DEPS_DIR}/llvm.sh
${DEPS_DIR}/llvm.sh 12
