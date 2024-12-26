#!/usr/bin/python3

# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

import argparse
import json
from hashlib import sha256
from pathlib import PurePath

MANIFEST_FILENAME = "org.equeim.Tremotesf.json"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("tag",
                        help="Git tag")
    parser.add_argument("archive",
                        help="Release archive filename")
    args = parser.parse_args()
    with open(MANIFEST_FILENAME, "r", encoding="utf-8") as f:
        manifest = json.load(f)
    tremotesf_module = next(module for module in manifest["modules"] if module["name"] == "tremotesf")
    del tremotesf_module["build-options"]["env"]["ASAN_OPTIONS"]
    tremotesf_module["config-opts"].remove("-DTREMOTESF_ASAN=ON")

    with open(args.archive, "rb") as f:
        archive_sha256 = sha256(f.read()).hexdigest()

    tremotesf_module["sources"] = [
        {"type": "archive", "url": f"https://github.com/equeim/tremotesf2/releases/download/{args.tag}/{PurePath(args.archive).name}",
         "sha256": archive_sha256}]
    with open(MANIFEST_FILENAME, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=4)


if __name__ == "__main__":
    main()
