# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import io
import os
import subprocess
import sys
import tarfile
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from git_archive_with_submodules import ArchiveError, write_archive

FILE = "100644"
EXECUTABLE = "100755"
SYMLINK = "120000"
SUBMODULE = "160000"
# Long enough that git archive needs an extra header record for the name.
LONG_NAME = "f" * 120 + ".txt"

SCRIPT = ROOT / "scripts" / "git_archive_with_submodules.py"

# A member is its name, its info and its content, or None for anything that
# is not a regular file.
Member = tuple[str, tarfile.TarInfo, bytes | None]


@pytest.fixture(autouse=True)
def isolated_git_config(monkeypatch):
    """Keeps the user's and the system's git settings out of every git call."""
    monkeypatch.setenv("GIT_CONFIG_GLOBAL", os.devnull)
    monkeypatch.setenv("GIT_CONFIG_NOSYSTEM", "1")


def git(
    repo: Path,
    *args: str,
    content: bytes = b"",
    extra_env: dict[str, str] | None = None,
) -> str:
    """Runs git in repo, ignoring the user's and the system's git settings."""
    env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")} | {
        "GIT_CONFIG_GLOBAL": os.devnull,
        "GIT_CONFIG_NOSYSTEM": "1",
        "GIT_AUTHOR_NAME": "Test",
        "GIT_AUTHOR_EMAIL": "test@example.com",
        "GIT_COMMITTER_NAME": "Test",
        "GIT_COMMITTER_EMAIL": "test@example.com",
    } | (extra_env or {})
    result = subprocess.run(
        ["git", "-C", str(repo), *args],
        input=content,
        capture_output=True,
        check=True,
        env=env,
    )
    return result.stdout.decode().strip()


def make_repo(path: Path) -> Path:
    """Creates an empty git repository at path."""
    path.mkdir(parents=True, exist_ok=True)
    git(path, "init", "-q", "-b", "main")
    return path


def commit(
    repo: Path,
    entries: dict[str, tuple[str, bytes]] | None = None,
    gitlinks: dict[str, str] | None = None,
) -> str:
    """Adds a commit to repo and returns its id.

    Args:
        repo: Repository to commit to.
        entries: Files by path, each as a git mode and content.
        gitlinks: Submodules by path, each as the commit they record.
    """
    index = {"GIT_INDEX_FILE": str(repo / "test-index")}
    for path, (mode, content) in (entries or {}).items():
        blob = git(repo, "hash-object", "-w", "--stdin", content=content)
        entry = f"{mode},{blob},{path}"
        git(repo, "update-index", "--add", "--cacheinfo", entry, extra_env=index)
    for path, submodule_commit in (gitlinks or {}).items():
        entry = f"{SUBMODULE},{submodule_commit},{path}"
        git(repo, "update-index", "--add", "--cacheinfo", entry, extra_env=index)
    tree = git(repo, "write-tree", extra_env=index)
    commit_id = git(repo, "commit-tree", tree, "-m", "test")
    git(repo, "update-ref", "HEAD", commit_id)
    return commit_id


def archive_bytes(repo: Path, tree_ish: str, prefix: str = "") -> bytes:
    """Returns the archive write_archive writes for tree_ish."""
    output = io.BytesIO()
    write_archive(repo, tree_ish, prefix, output)
    return output.getvalue()


def raw_archive(repo: Path, tree_ish: str, prefix: str) -> bytes:
    """Returns what git archive alone writes for tree_ish."""
    return subprocess.run(
        [
            "git",
            "-C",
            str(repo),
            "archive",
            "--format=tar",
            f"--prefix={prefix}",
            tree_ish,
        ],
        capture_output=True,
        check=True,
    ).stdout


def members_of(archive: tarfile.TarFile) -> list[Member]:
    """Returns each member of archive, in archive order."""
    members = []
    for member in archive:
        data = archive.extractfile(member).read() if member.isreg() else None
        members.append((member.name, member, data))
    return members


def read_archive(repo: Path, tree_ish: str, prefix: str = "") -> list[Member]:
    """Returns the members of the archive of tree_ish, in archive order."""
    output = io.BytesIO(archive_bytes(repo, tree_ish, prefix))
    with tarfile.open(fileobj=output) as archive:
        return members_of(archive)


def names(members: list[Member]) -> list[str]:
    """Returns the sorted member names, with duplicates kept."""
    return sorted(name for name, _, _ in members)


def test_copies_tree_and_submodules_at_pinned_commits(tmp_path):
    inner = make_repo(tmp_path / "super/ext/sub/inner")
    inner_commit = commit(inner, {"nested.txt": (FILE, b"inner\n")})

    sub = make_repo(tmp_path / "super/ext/sub")
    pinned_commit = commit(
        sub,
        {"sub.txt": (FILE, b"pinned\n"), LONG_NAME: (FILE, b"long\n")},
        gitlinks={"inner": inner_commit},
    )
    commit(sub, {"sub.txt": (FILE, b"later\n")})

    top = make_repo(tmp_path / "super")
    top_commit = commit(
        top,
        {
            "main.txt": (FILE, b"main\n"),
            "run.sh": (EXECUTABLE, b"#!/bin/sh\n"),
            "link": (SYMLINK, b"main.txt"),
        },
        gitlinks={"ext/sub": pinned_commit},
    )

    members = read_archive(top, top_commit, "zrythm-test/")

    assert names(members) == [
        "zrythm-test",
        "zrythm-test/ext",
        "zrythm-test/ext/sub",
        "zrythm-test/ext/sub/" + LONG_NAME,
        "zrythm-test/ext/sub/inner",
        "zrythm-test/ext/sub/inner/nested.txt",
        "zrythm-test/ext/sub/sub.txt",
        "zrythm-test/link",
        "zrythm-test/main.txt",
        "zrythm-test/run.sh",
    ]
    assert {name: data for name, _, data in members if data is not None} == {
        "zrythm-test/main.txt": b"main\n",
        "zrythm-test/run.sh": b"#!/bin/sh\n",
        "zrythm-test/ext/sub/sub.txt": b"pinned\n",
        "zrythm-test/ext/sub/" + LONG_NAME: b"long\n",
        "zrythm-test/ext/sub/inner/nested.txt": b"inner\n",
    }
    info = {name: member for name, member, _ in members}
    assert info["zrythm-test/main.txt"].mode == 0o664
    assert info["zrythm-test/run.sh"].mode == 0o775
    assert info["zrythm-test/link"].issym()
    assert info["zrythm-test/link"].linkname == "main.txt"
    # the archive holds git's records only, with nothing added
    plain = sum(
        len(raw_archive(repo, rev, prefix))
        for repo, rev, prefix in (
            (top, top_commit, "zrythm-test/"),
            (sub, pinned_commit, "zrythm-test/ext/sub/"),
            (inner, inner_commit, "zrythm-test/ext/sub/inner/"),
        )
    )
    assert len(archive_bytes(top, top_commit, "zrythm-test/")) <= plain


def test_empty_submodule_is_an_empty_directory(tmp_path):
    sub = make_repo(tmp_path / "super/ext/sub")
    empty_commit = commit(sub)
    top = make_repo(tmp_path / "super")
    top_commit = commit(
        top, {"main.txt": (FILE, b"main\n")}, gitlinks={"ext/sub": empty_commit}
    )

    members = read_archive(top, top_commit, "zrythm-test/")

    assert names(members) == [
        "zrythm-test",
        "zrythm-test/ext",
        "zrythm-test/ext/sub",
        "zrythm-test/main.txt",
    ]
    assert {name: member for name, member, _ in members}["zrythm-test/ext/sub"].isdir()


def test_fails_when_submodule_commit_is_missing(tmp_path):
    sub = make_repo(tmp_path / "super/ext/sub")
    commit(sub, {"sub.txt": (FILE, b"sub\n")})
    missing_commit = "0123456789012345678901234567890123456789"
    top = make_repo(tmp_path / "super")
    top_commit = commit(top, gitlinks={"ext/sub": missing_commit})

    with pytest.raises(ArchiveError) as excinfo:
        read_archive(top, top_commit)

    assert "submodule ext/sub" in str(excinfo.value)
    assert missing_commit in str(excinfo.value)


def test_fails_when_submodule_is_not_checked_out(tmp_path):
    elsewhere = make_repo(tmp_path / "elsewhere")
    orphan_commit = commit(elsewhere, {"a.txt": (FILE, b"a\n")})
    top = make_repo(tmp_path / "super")
    top_commit = commit(top, gitlinks={"ext/sub": orphan_commit})

    with pytest.raises(ArchiveError) as excinfo:
        read_archive(top, top_commit)

    assert "submodule ext/sub" in str(excinfo.value)
    assert orphan_commit in str(excinfo.value)


def test_fails_when_submodule_directory_is_not_a_checkout(tmp_path):
    top = make_repo(tmp_path / "super")
    # A clone without submodules leaves an empty directory in their place.
    (top / "ext" / "sub").mkdir(parents=True)
    own_commit = commit(top, {"a.txt": (FILE, b"a\n")})
    top_commit = commit(
        top, {"main.txt": (FILE, b"main\n")}, gitlinks={"ext/sub": own_commit}
    )

    with pytest.raises(ArchiveError) as excinfo:
        read_archive(top, top_commit)

    assert "submodule ext/sub" in str(excinfo.value)
    assert own_commit in str(excinfo.value)


def test_fails_on_unknown_tree(tmp_path):
    top = make_repo(tmp_path)

    with pytest.raises(ArchiveError, match="no-such-rev"):
        read_archive(top, "no-such-rev")


def test_cli_writes_the_tar_to_stdout(tmp_path):
    top = make_repo(tmp_path / "super")
    top_commit = commit(top, {"main.txt": (FILE, b"main\n")})

    result = subprocess.run(
        [sys.executable, str(SCRIPT), "--prefix=p/", top_commit],
        cwd=top,
        capture_output=True,
        check=True,
    )

    with tarfile.open(fileobj=io.BytesIO(result.stdout)) as archive:
        assert [(name, data) for name, _, data in members_of(archive)] == [
            ("p", None),
            ("p/main.txt", b"main\n"),
        ]


def test_cli_reports_errors_on_stderr_with_exit_code_1(tmp_path):
    top = make_repo(tmp_path)

    result = subprocess.run(
        [sys.executable, str(SCRIPT), "no-such-rev"],
        cwd=top,
        capture_output=True,
        check=False,
    )

    assert result.returncode == 1
    assert "error: git archive failed for no-such-rev" in result.stderr.decode()


def test_cli_rejects_prefix_without_trailing_slash(tmp_path):
    result = subprocess.run(
        [sys.executable, str(SCRIPT), "--prefix=p", "HEAD"],
        cwd=tmp_path,
        capture_output=True,
        check=False,
    )

    assert result.returncode == 2
    assert b"--prefix must end with '/'" in result.stderr
