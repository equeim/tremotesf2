#!/usr/bin/python3

# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

import argparse
import json
import os
import subprocess
import sys
from enum import Enum
from pathlib import Path
import logging


def get_project_version() -> str:
    cmakelists = "CMakeLists.txt"
    if not Path(cmakelists).exists():
        raise RuntimeError(f"{cmakelists} doesn't exist")
    process = subprocess.run(["cmake", "--trace-format=json-v1", "--trace-expand", "-P", cmakelists],
                             stdout=subprocess.DEVNULL,
                             stderr=subprocess.PIPE,
                             text=True)
    for line in process.stderr.splitlines():
        trace = json.loads(line)
        if "cmd" in trace and trace["cmd"].lower() == "project":
            args = trace["args"]
            found_version = False
            for arg in args:
                if found_version:
                    return arg
                elif arg.lower() == "version":
                    found_version = True
    raise RuntimeError("Failed to find version in CMake trace output")


def make_tar_archive(debian: bool) -> str:
    version = get_project_version()
    root_directory = f"tremotesf-{version}"
    if debian:
        archive_filename = f"tremotesf_{version}.orig.tar"
    else:
        archive_filename = f"{root_directory}.tar"
    logging.info(f"Making tar archive {archive_filename}")
    files = subprocess.run(["git", "ls-files", "--recurse-submodules"],
                           check=True,
                           stdout=subprocess.PIPE,
                           text=True).stdout.splitlines()
    logging.info(f"Archiving {len(files)} files")
    subprocess.run(["tar", "--create", "--file", archive_filename, "--transform", f"s,^,{root_directory}/,"] + files, check=True, stdout=sys.stderr)
    return archive_filename


def compress_gzip(path: str) -> str:
    compressed_path = f"{path}.gz"
    logging.info(f"Compressing {path} to {compressed_path}")
    subprocess.run(["gzip", "-k", path], check=True, stdout=sys.stderr)
    return compressed_path


def compress_zstd(path: str) -> str:
    compressed_path = f"{path}.zst"
    logging.info(f"Compressing {path} to {compressed_path}")
    subprocess.run(["zstd", path, "-o", compressed_path], check=True, stdout=sys.stderr)
    return compressed_path


class CompressionType(Enum):
    ALL = "all"
    GZIP = "gzip"
    ZSTD = "zstd"


logging.basicConfig(level=logging.INFO, stream=sys.stderr)

parser = argparse.ArgumentParser()
parser.add_argument('compression',
                    choices=[member.value for member in list(CompressionType)],
                    help="Compression type")
parser.add_argument("--debian",
                    action="store_true",
                    help="Make Debian upstream tarball")

args = parser.parse_args()
compression_type = CompressionType(args.compression)

tar = make_tar_archive(args.debian)
if compression_type == CompressionType.ALL:
    print(compress_gzip(tar))
    print(compress_zstd(tar))
elif compression_type == CompressionType.GZIP:
    print(compress_gzip(tar))
elif compression_type == CompressionType.ZSTD:
    print(compress_zstd(tar))
logging.info(f"Removing {tar}")
os.remove(tar)
