#!/bin/bash

# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

exec find src -path src/3rdparty -prune -or \( \( -name '*.cpp' -or -name '*.h' \) -and -not -name 'recoloringsvgiconengine.*' \) -exec clang-format --verbose -i {} +
