#!/bin/sh

if command -v lupdate-qt5 > /dev/null 2>&1; then
    _lupdate=lupdate-qt5
else
    _lupdate=lupdate
fi

_dir="$(realpath $(dirname $0))"
cd "$_dir"

QT_SELECT=5 $_lupdate ../src ../qml -ts en.ts \
                                        es.ts \
                                        nl_BE.ts \
                                        nl.ts \
                                        ru.ts \
                                        source.ts
