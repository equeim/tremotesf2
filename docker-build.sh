#!/usr/bin/bash

# SPDX-FileCopyrightText: 2026 ds
#
# SPDX-License-Identifier: CC0-1.0

set -e

rm -rf build-out
docker build --tag tremotesf-build "$@" .
mkdir -p build-out
id=$(docker create tremotesf-build)
docker cp $id:/src/build/src/tremotesf build-out/
docker cp $id:/src/build/tremotesf.appimage build-out/
docker rm $id
docker image rm tremotesf-build

HI='\033[0;32m'
NC='\033[0m' # No Color

echo
echo  =^..^=   =^..^=   =^..^=    =^..^=    =^..^=    =^..^=    =^..^=    =^..^=    =^..^=
echo
echo -e Output is at ${HI}./build-out${NC} folder.
echo -e "The build is FULLY static (musl): no shared libraries, no libc, no"
echo -e "interpreter - it runs on any x86-64/aarch64 Linux, glibc or musl."
echo -e "It is static, Wayland only and software rendered - no OpenGL/EGL/mesa."
echo -e "D-Bus is linked in, so it picks up the KDE/Plasma theme."
echo -e "At runtime it needs a wayland compositor, nothing else."
echo -e "There is also a tremotesf.appimage in the output folder."
