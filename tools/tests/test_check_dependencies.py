# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from dependency_manifest import (
    scan_conan_requirements,
    scan_cpm_declared_versions,
    scan_cpm_fetches,
    scan_vendored_dirs,
)

REPO_ROOT = Path(__file__).resolve().parents[2]


def test_scan_conan_requirements(tmp_path):
    conanfile = tmp_path / "conanfile.py"
    conanfile.write_text(
        "class Z(ConanFile):\n"
        "    def requirements(self):\n"
        "        self.requires('qt/[>=6.11.1]')\n"
        '        self.requires("fmt/[~12]")\n'
        "        if self.settings.os == 'Linux':\n"
        "            self.requires('freetype/[>=2.14]')\n"
        "    def build_requirements(self):\n"
        "        self.tool_requires('cmake/[>=4.3]')\n"
        "        self.test_requires('gtest/[~1.16]')\n"
    )
    assert scan_conan_requirements(conanfile) == {
        "qt", "fmt", "freetype", "cmake", "gtest",
    }


def test_scan_cpm_fetches_skips_comments(tmp_path):
    cmake = tmp_path / "CMakeLists.txt"
    cmake.write_text(
        "CPMGetPackage(clap)\n"
        "CPMGetPackage(farbot )\n"
        "# CPMGetPackage(fast_float) # dependency of scn\n"
    )
    assert scan_cpm_fetches([cmake]) == {"clap", "farbot"}


def test_scan_cpm_declared_versions(tmp_path):
    lock = tmp_path / "package-lock.cmake"
    lock.write_text(
        "CPMDeclarePackage(clap\n"
        "  NAME clap\n"
        "  VERSION 1.2.7\n"
        "  GIT_TAG abc\n"
        ")\n"
        "CPMDeclarePackage(juce\n"
        "  NAME juce\n"
        "  VERSION 9.0.0\n"
        "  GIT_TAG def\n"
        ")\n"
    )
    assert scan_cpm_declared_versions(lock) == {
        "clap": "1.2.7",
        "juce": "9.0.0",
    }


def test_scan_cpm_declared_versions_tolerates_parens_in_fields(tmp_path):
    lock = tmp_path / "package-lock.cmake"
    lock.write_text(
        "CPMDeclarePackage(clap\n"
        "  NAME clap\n"
        '  OPTIONS "SOME_FLAG(1)"\n'
        "  VERSION 1.2.7\n"
        ")\n"
    )
    assert scan_cpm_declared_versions(lock) == {"clap": "1.2.7"}


def test_scan_vendored_dirs(tmp_path):
    ext = tmp_path / "ext"
    (ext / "rubberband").mkdir(parents=True)
    (ext / "soxr").mkdir()
    (ext / "README").write_text("x")
    assert scan_vendored_dirs(ext) == {"rubberband", "soxr"}


def test_real_repo_scans_match_manifest():
    from dependency_manifest import load_manifest

    manifest = load_manifest(
        REPO_ROOT / "data" / "dependencies.toml",
        [REPO_ROOT / "LICENSES", REPO_ROOT / "data" / "licenses"],
    )
    by_id = manifest.by_id()
    conan = scan_conan_requirements(REPO_ROOT / "conanfile.py")
    cpm = scan_cpm_fetches(
        [REPO_ROOT / "CMakeLists.txt", REPO_ROOT / "ext" / "CMakeLists.txt"]
    )
    vendored = scan_vendored_dirs(REPO_ROOT / "ext")
    for name in conan:
        assert name in by_id and by_id[name].tier == "conan", name
    for name in cpm:
        assert name in by_id and by_id[name].tier == "cpm", name
    for name in vendored:
        assert name in by_id and by_id[name].tier == "vendored", name
