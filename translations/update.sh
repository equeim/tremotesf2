#!/bin/bash

# SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

_dir="$(realpath $(dirname $0))"
cd "$_dir"

lupdate-qt5 ../src/tremotesf -no-obsolete -ts en.ts \
                                              es.ts \
                                              fr.ts \
                                              it_IT.ts \
                                              nl_BE.ts \
                                              nl.ts \
                                              pl.ts \
                                              ru.ts \
                                              zh_CN.ts \
                                              source.ts
