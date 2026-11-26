#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Writes a tar archive of a git tree together with its submodules.

git archive leaves submodule directories empty. This script appends the
archive of each submodule, below the submodule's path, at the commit the
given tree records. Nested submodules are handled the same way. Paths with
the export-ignore attribute are left out, as in every git archive. The
uncompressed tar goes to stdout.

Usage: git_archive_with_submodules.py [--prefix PREFIX/] <tree-ish>

Every submodule that goes into the archive must be checked out and have its
recorded commit available. Git 2.40 or newer is required.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import BinaryIO

BLOCK_SIZE = 512
RECORD_SIZE = 10240
END_OF_ARCHIVE = b"\0" * BLOCK_SIZE
# Layout of a header block, and the record types git archive writes.
TYPE_FLAG = slice(156, 157)
SIZE_FIELD = slice(124, 136)
DIRECTORY = b"5"
GLOBAL_HEADER = b"g"
# Records that carry the name or the link target of the member after them.
MEMBER_HEADER_TYPES = b"xLK"

SUBMODULE_MODE = b"160000"


class ArchiveError(Exception):
    """A tree or one of its submodules cannot be archived."""


def write_archive(repo: Path, tree_ish: str, prefix: str, output: BinaryIO) -> None:
    """Writes the uncompressed tar of tree_ish, with its submodules, to output.

    The output is the git archive of tree_ish with the archive of each
    submodule placed after it, below the submodule's path. Submodules under
    a path with the export-ignore attribute are left out as well.

    Args:
        repo: Root of the git checkout holding tree_ish.
        tree_ish: Commit, tag or tree to archive.
        prefix: Directory path ending with a slash, put in front of every
            archive member. Empty for none.
        output: Stream the tar is written to.

    Raises:
        ArchiveError: A submodule is not checked out or lacks its recorded
            commit, or git archive fails.
    """
    written = _append_tree(repo, tree_ish, prefix, output, skip_root=False)
    end = END_OF_ARCHIVE * 2
    output.write(end + b"\0" * (-(written + len(end)) % RECORD_SIZE))
    output.flush()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--prefix",
        default="",
        help="directory path put in front of every member, ending with '/'",
    )
    parser.add_argument("tree_ish", help="commit, tag or tree to archive")
    args = parser.parse_args(argv)
    if args.prefix and not args.prefix.endswith("/"):
        parser.error("--prefix must end with '/'")
    try:
        write_archive(Path.cwd(), args.tree_ish, args.prefix, sys.stdout.buffer)
    except ArchiveError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


def _append_tree(
    repo: Path, tree_ish: str, prefix: str, output: BinaryIO, skip_root: bool
) -> int:
    """Appends the archive of tree_ish in repo to output, then its submodules'.

    The directory entry that git archive puts first is left out when
    skip_root is set: the parent tree already holds that entry.

    Returns:
        The number of bytes written.
    """
    written = _copy_archive(repo, tree_ish, prefix, output, skip_root)
    for path, commit in _submodules(repo, tree_ish):
        submodule = repo / path
        _check_commit(submodule, path, commit)
        written += _append_tree(
            submodule, commit, f"{prefix}{path}/", output, skip_root=True
        )
    return written


def _copy_archive(
    repo: Path, tree_ish: str, prefix: str, output: BinaryIO, skip_root: bool
) -> int:
    """Copies the git archive of tree_ish in repo to output.

    The end-of-archive marker is left out.

    Returns:
        The number of bytes written.
    """
    archive = subprocess.Popen(
        ["git", "archive", "--format=tar", f"--prefix={prefix}", tree_ish],
        cwd=repo,
        stdout=subprocess.PIPE,
    )
    try:
        written = _copy_members(archive.stdout, output, skip_root)
        failed = archive.wait() != 0
    except ValueError as e:
        raise ArchiveError(
            f"git archive wrote an unreadable tar header for {tree_ish} in {repo}"
        ) from e
    finally:
        archive.stdout.close()
        if archive.poll() is None:
            archive.kill()
        archive.wait()
    if failed:
        # git reports the reason on stderr
        raise ArchiveError(f"git archive failed for {tree_ish} in {repo}")
    return written


def _copy_members(source: BinaryIO, dest: BinaryIO, skip_root: bool) -> int:
    """Copies the tar members read from source to dest, up to the end marker.

    The first member is left out when skip_root is set. Header records go to
    dest together with the member they belong to, so a header record without
    a member after it is left out too.

    Returns:
        The number of bytes written.
    """
    written = 0
    global_records = bytearray()
    member_records = bytearray()
    while True:
        header = source.read(BLOCK_SIZE)
        if len(header) < BLOCK_SIZE or header == END_OF_ARCHIVE:
            return written
        content = source.read(_content_size(header))
        kind = header[TYPE_FLAG]
        if kind == GLOBAL_HEADER:
            global_records += header + content
        elif kind in MEMBER_HEADER_TYPES:
            member_records += header + content
        elif skip_root:
            skip_root = False
            if kind != DIRECTORY:
                raise ArchiveError("git archive does not start with a directory entry")
            member_records.clear()
        else:
            records = global_records + member_records
            global_records.clear()
            member_records.clear()
            records += header + content
            written += len(records)
            dest.write(records)


def _content_size(header: bytes) -> int:
    """Returns the size of the content after a tar header, in whole blocks."""
    size = int(header[SIZE_FIELD].rstrip(b"\0 ") or b"0", 8)
    return -(-size // BLOCK_SIZE) * BLOCK_SIZE


def _submodules(repo: Path, tree_ish: str) -> list[tuple[str, str]]:
    """Returns the path and commit of every submodule the archive holds.

    A submodule under a path with the export-ignore attribute is left out.
    """
    submodules = []
    for record in _git(repo, "ls-tree", "-r", "-z", tree_ish).split(b"\0"):
        if not record:
            continue
        metadata, _, path = record.partition(b"\t")
        mode, _, commit = metadata.split(b" ")
        if mode == SUBMODULE_MODE:
            submodules.append((os.fsdecode(path), commit.decode("ascii")))
    ignored = _export_ignored(repo, tree_ish, [path for path, _ in submodules])
    return [(path, commit) for path, commit in submodules if path not in ignored]


def _export_ignored(repo: Path, tree_ish: str, paths: list[str]) -> set[str]:
    """Returns the paths that git archive leaves out of tree_ish.

    A path is left out when it or a directory above it has the export-ignore
    attribute.
    """
    if not paths:
        return set()
    queried = set()
    for path in paths:
        queried.update(_path_and_directories_above(path))
    listing = _git(
        repo,
        "check-attr",
        "--source",
        tree_ish,
        "-z",
        "--stdin",
        "export-ignore",
        input=b"\0".join(os.fsencode(path) for path in sorted(queried)) + b"\0",
    )
    # -z prints path, attribute and value as NUL-separated triples
    fields = listing.split(b"\0")
    marked = {
        os.fsdecode(path)
        for path, value in zip(fields[0::3], fields[2::3])
        if value == b"set"
    }
    return {path for path in paths if _path_and_directories_above(path) & marked}


def _path_and_directories_above(path: str) -> set[str]:
    """Returns path together with every directory above it."""
    parts = path.split("/")
    return {"/".join(parts[:end]) for end in range(1, len(parts) + 1)}


def _git(repo: Path, *args: str, input: bytes = b"") -> bytes:
    """Runs git in repo with input on its stdin and returns its stdout."""
    result = subprocess.run(
        ["git", *args], cwd=repo, input=input, capture_output=True, check=False
    )
    if result.returncode != 0:
        detail = result.stderr.decode(errors="replace").strip()
        raise ArchiveError(f"git {' '.join(args)} failed in {repo}: {detail}")
    return result.stdout


def _check_commit(repo: Path, path: str, commit: str) -> None:
    """Raises ArchiveError unless repo is a git checkout that holds commit.

    Args:
        repo: Directory of a submodule.
        path: Path of the submodule, for the error message.
        commit: Commit the parent tree records for the submodule.
    """
    # A directory without a .git of its own makes git answer for the parent
    # repository.
    if (repo / ".git").exists():
        result = subprocess.run(
            ["git", "cat-file", "-e", f"{commit}^{{commit}}"],
            cwd=repo,
            capture_output=True,
            check=False,
        )
        if result.returncode == 0:
            return
    raise ArchiveError(
        f"submodule {path} is not checked out or does not contain commit {commit}. "
        "Run 'git submodule update --init --recursive' in the top-level checkout."
    )


if __name__ == "__main__":
    sys.exit(main())
