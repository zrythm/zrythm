# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from check_dependencies import find_drift
from dependency_manifest import (
    scan_conan_enabled_options,
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


def test_scan_conan_enabled_options(tmp_path):
    conanfile = tmp_path / "conanfile.py"
    conanfile.write_text(
        "class Z(ConanFile):\n"
        "    def requirements(self):\n"
        "        self.options['qt'].qthttpserver = True\n"
        '        self.options["qt"].qtgrpc = False\n'
        "        self.options['qt'].multithreaded = enabled\n"
        "        self.conf['user.pkg:skip'] = True\n"
        "        other.options['qt'].qtsvg = True\n"
    )
    assert scan_conan_enabled_options(conanfile) == {("qt", "qthttpserver")}


def _manifest_with_part(tmp_path: Path):
    from dependency_manifest import load_manifest

    (tmp_path / "MIT.txt").write_text("text")
    manifest = tmp_path / "dependencies.toml"
    manifest.write_text(
        '[[component]]\nid="qt"\nname="Qt"\ntier="conan"\nspdx="MIT"\n'
        'copyright="c"\nhomepage="https://x"\n'
        '[[component]]\nid="qthttpserver"\nname="Qt HTTP Server"\n'
        'tier="conan"\npart_of="qt"\nspdx="MIT"\ncopyright="c"\n'
        'homepage="https://x"\n'
    )
    return load_manifest(manifest, [tmp_path])


def _part_scans() -> dict[str, set[str]]:
    return {"conan": {"qt"}, "cpm": set(), "vendored": set()}


def test_part_with_enabled_module_option_has_no_drift(tmp_path):
    errors = find_drift(
        _manifest_with_part(tmp_path),
        _part_scans(),
        enabled_options={("qt", "qthttpserver")},
        declared_versions={},
    )
    assert errors == []


def test_part_without_enabled_module_option_is_drift(tmp_path):
    errors = find_drift(
        _manifest_with_part(tmp_path),
        _part_scans(),
        enabled_options=set(),
        declared_versions={},
    )
    assert errors == [
        "'qthttpserver' is part of 'qt' but conanfile.py does not set "
        'self.options["qt"].qthttpserver = True'
    ]
