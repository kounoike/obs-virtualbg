#!/bin/bash

set -xe

cd $(dirname $0)/..
DEPS_DIR=$(pwd)/deps

OBS_VER="$(dpkg -s obs-studio | gawk '$1=="Version:"{print gensub(/^([0-9]*\.[0-9]*)\..*/, "\\1.0", "g", $2)}')"

# TODO: set LINUX_MAINTAINER_EMAIL

mkdir package

sudo apt-get install checkinstall

cd build
PAGER="cat" sudo checkinstall -y --type=debian --fstrans=no --nodoc \
	--backup=no --deldoc=yes --install=no \
	--pkgname="obs-virtualbg" --pkgversion="$VERSION" \
	--pkglicense="MIT" --maintainer="$LINUX_MAINTAINER_EMAIL" \
	--pkggroup="video" \
	--requires="obs-studio \(\>= ${OBS_VER}\)" \
	--pakdir="../package"

