#!/bin/bash

# SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

exec find src/tremotesf \( -name '*.cpp' -or -name '*.h' \) -not -name 'recoloringsvgiconengine.*' -exec clang-format --verbose -i {} +
