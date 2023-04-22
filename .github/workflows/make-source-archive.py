#!/usr/bin/python3

# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

import argparse
import gzip
import json
import logging
import shutil
import subprocess
import sys
import tarfile
from enum import Enum
from pathlib import Path, PurePath
from tempfile import TemporaryDirectory


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


def make_tar_archive(tempdir: PurePath, debian: bool) -> PurePath:
    version = get_project_version()
    root_directory = f"tremotesf-{version}"
    if debian:
        archive_filename = f"tremotesf_{version}.orig.tar"
    else:
        archive_filename = f"tremotesf-{version}.tar"
    archive_path = tempdir / archive_filename
    logging.info(f"Making tar archive {archive_path}")
    files = subprocess.run(["git", "ls-files", "--recurse-submodules"],
                           check=True,
                           stdout=subprocess.PIPE,
                           text=True).stdout.splitlines()
    logging.info(f"Archiving {len(files)} files")
    with tarfile.open(archive_path, mode="x") as tar:
        for file in files:
            tar.add(file, arcname=str(PurePath(root_directory, file)), recursive=False)
    return archive_path


def compress_gzip(output_directory: PurePath, tar_path: PurePath) -> PurePath:
    gzip_path = output_directory / tar_path.with_suffix(".tar.gz").name
    logging.info(f"Compressing {tar_path} to {gzip_path}")
    with open(tar_path, mode="rb") as tar_file, gzip.open(gzip_path, mode="xb") as gzip_file:
        shutil.copyfileobj(tar_file, gzip_file)
    return gzip_path


def compress_zstd(output_directory: PurePath, tar_path: PurePath) -> PurePath:
    zstd_path = output_directory / tar_path.with_suffix(".tar.zst").name
    logging.info(f"Compressing {tar_path} to {zstd_path}")
    subprocess.run(["zstd", str(tar_path), "-o", str(zstd_path)], check=True, stdout=sys.stderr)
    return zstd_path


class CompressionType(Enum):
    ALL = "all"
    GZIP = "gzip"
    ZSTD = "zstd"


def main():
    logging.basicConfig(level=logging.INFO, stream=sys.stderr)

    parser = argparse.ArgumentParser()
    parser.add_argument('compression',
                        choices=[member.value for member in list(CompressionType)],
                        help="Compression type")
    parser.add_argument("--output-directory",
                        help="Directory where to place archives. Defaults to current directory")
    parser.add_argument("--debian",
                        action="store_true",
                        help="Make Debian upstream tarball")

    args = parser.parse_args()
    compression_type = CompressionType(args.compression)
    if args.output_directory:
        output_directory = PurePath(args.output_directory)
    else:
        output_directory = Path.cwd()

    with TemporaryDirectory() as tempdir:
        tar = make_tar_archive(PurePath(tempdir), args.debian)
        if compression_type == CompressionType.ALL:
            print(compress_gzip(output_directory, tar))
            print(compress_zstd(output_directory, tar))
        elif compression_type == CompressionType.GZIP:
            print(compress_gzip(output_directory, tar))
        elif compression_type == CompressionType.ZSTD:
            print(compress_zstd(output_directory, tar))


if __name__ == "__main__":
    main()
