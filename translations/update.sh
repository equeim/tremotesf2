#!/bin/sh

# SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if command -v lupdate-qt5 > /dev/null 2>&1; then
    _lupdate=lupdate-qt5
else
    _lupdate=lupdate
fi

_dir="$(realpath $(dirname $0))"
cd "$_dir"

QT_SELECT=5 $_lupdate ../src ../qml -ts en.ts \
                                        es.ts \
                                        fr.ts \
                                        it_IT.ts \
                                        nl_BE.ts \
                                        nl.ts \
                                        pl.ts \
                                        ru.ts \
                                        zh_CN.ts \
                                        source.ts
