#!/bin/bash

# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

set -eo pipefail

readonly SCRIPT_DIR="$(realpath "$(dirname "$0")")"

readonly ICONSET="${SCRIPT_DIR}/tremotesf.iconset"
readonly HICOLOR_ICONS_DIR="${SCRIPT_DIR}/../hicolor"
readonly HICOLOR_ICON_NAME="org.equeim.Tremotesf"
readonly HICOLOR_ICON_PNG="${HICOLOR_ICON_NAME}.png"
readonly HICOLOR_ICON_SVG="${HICOLOR_ICONS_DIR}/scalable/apps/${HICOLOR_ICON_NAME}.svg"

rm -rf "${ICONSET}"
mkdir -p "${ICONSET}"

cp "${HICOLOR_ICONS_DIR}/16x16/apps/${HICOLOR_ICON_PNG}" "${ICONSET}/icon_16x16.png"
cp "${HICOLOR_ICONS_DIR}/32x32/apps/${HICOLOR_ICON_PNG}" "${ICONSET}/icon_16x16@2x.png"
cp "${HICOLOR_ICONS_DIR}/32x32/apps/${HICOLOR_ICON_PNG}" "${ICONSET}/icon_32x32.png"
inkscape -w 64 -h 64 "$HICOLOR_ICON_SVG" -o "${ICONSET}/icon_32x32@2x.png"
inkscape -w 128 -h 128 "$HICOLOR_ICON_SVG" -o "${ICONSET}/icon_128x128.png"
inkscape -w 256 -h 256 "$HICOLOR_ICON_SVG" -o "${ICONSET}/icon_128x128@2x.png"
inkscape -w 256 -h 256 "$HICOLOR_ICON_SVG" -o "${ICONSET}/icon_256x256.png"
inkscape -w 512 -h 512 "$HICOLOR_ICON_SVG" -o "${ICONSET}/icon_256x256@2x.png"
inkscape -w 512 -h 512 "$HICOLOR_ICON_SVG" -o "${ICONSET}/icon_512x512.png"
inkscape -w 1024 -h 1024 "$HICOLOR_ICON_SVG" -o "${ICONSET}/icon_512x512@2x.png"

iconutil -c icns "$ICONSET"

rm -rf "${ICONSET}"
