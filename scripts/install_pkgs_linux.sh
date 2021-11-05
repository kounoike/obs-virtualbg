#!/bin/bash

add-apt-repository ppa:obsproject/obs-studio
apt-get update
apt-get install -y obs-studio
apt-get install -y python3-pip
pip3 install cmake

DEPS_DIR=$(cd $(dirname $0)/..;pwd)/deps
[ -d ${DEPS_DIR} ] || mkdir ${DEPS_DIR}

curl -L -o ${DEPS_DIR}/llvm.sh https://apt.llvm.org/llvm.sh
chmod +x ${DEPS_DIR}/llvm.sh
${DEPS_DIR}/llvm.sh 12
apt-get install libc++-12-dev libc++1-12
