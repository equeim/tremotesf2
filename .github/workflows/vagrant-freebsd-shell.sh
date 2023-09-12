#!/bin/bash

# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

set -eo pipefail

readonly script_path="$1"
readonly script_filename="$(basename "${script_path}")"
export VAGRANT_CWD="$(dirname "$0")"
vagrant upload "${script_path}" "/tmp/${script_filename}"
exec vagrant ssh -c "cd /workspace && sh -e -o pipefail '/tmp/${script_filename}'"
