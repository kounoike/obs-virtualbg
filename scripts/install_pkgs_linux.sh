#!/bin/bash

add-apt-repository ppa:obsproject/obs-studio
apt-get update
apt-get install -y obs-studio
apt-get install -y python3-pip
pip3 install cmake

DEPS_DIR=$(cd $(dirname $0)/..;pwd)/deps
[ -d ${DEPS_DIR} ] || mkdir ${DEPS_DIR}
