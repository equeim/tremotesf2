#!/usr/bin/python3

# SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

import json
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


def make_tar_archive() -> str:
    root_directory = f"tremotesf-{get_project_version()}"
    archive_filename = f"{root_directory}.tar"
    logging.info(f"Making tar archive {archive_filename}")
    logging.info(f"Making archive for root directory")
    subprocess.run(["git", "archive", "--format=tar", f"--prefix={root_directory}/", "-o", archive_filename, "HEAD"],
                   check=True, stdout=sys.stderr)
    logging.info(f"Making archives for submodules")
    subprocess.run(["git", "submodule", "foreach",
                    f"echo \"Making archive for submodule $sm_path\"; git archive --format=tar --prefix=\"{root_directory}/$sm_path/\" -o 'archive.tar' HEAD"],
                   check=True, stdout=sys.stderr)
    submodule_archives = subprocess.run(["git", "submodule", "--quiet", "foreach", "echo \"$sm_path/archive.tar\""],
                                        check=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                        text=True).stdout.splitlines()
    logging.info(f"Concatenating {archive_filename} with {submodule_archives}")
    subprocess.run(["tar", "-f", archive_filename, "--concatenate"] + submodule_archives, check=True, stdout=sys.stderr)
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

compression_types_values = [member.value for member in list(CompressionType)]
try:
    compression_type = CompressionType(sys.argv[1])
except IndexError:
    print(
        f"Compression type must be specified as command-line option. Valid types are {compression_types_values}",
        file=sys.stderr)
    exit(1)
except ValueError:
    print(
        f"{sys.argv[1]} is not a valid compression type. Valid types are {compression_types_values}",
        file=sys.stderr)
    exit(1)

tar = make_tar_archive()
if compression_type == CompressionType.ALL:
    print(compress_gzip(tar))
    print(compress_zstd(tar))
elif compression_type == CompressionType.GZIP:
    print(compress_gzip(tar))
elif compression_type == CompressionType.ZSTD:
    print(compress_zstd(tar))
