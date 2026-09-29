# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from dependency_manifest import load_manifest
from generate_attributions import build_attributions

MANIFEST_TEXT = """
[[component]]
id = "qt"
name = "Qt"
tier = "conan"
spdx = "LGPL-3.0-only OR LicenseRef-Qt-Commercial"
copyright = "The Qt Company Ltd."
homepage = "https://www.qt.io"

[[component]]
id = "rubberband"
name = "Rubber Band Library"
tier = "vendored"
version = "4.0.0"
spdx = "GPL-2.0-or-later"
copyright = "Particular Programs Ltd"
homepage = "https://breakfastquay.com/rubberband/"

[[component]]
id = "gtest"
name = "GoogleTest"
tier = "conan"
spdx = "BSD-3-Clause"
copyright = "Google LLC"
homepage = "https://github.com/google/googletest"
shipped = false
"""

FRAGMENT = {
    "metadata": {"component": {"bom-ref": "root-ref"}},
    "components": [
        {
            "bom-ref": "ref-qt",
            "name": "qt",
            "version": "6.11.1",
            "licenses": [{"license": {"id": "LGPL-3.0-only"}}],
            "externalReferences": [
                {"type": "vcs", "url": "https://code.qt.io/qt5.git"},
                {"type": "website", "url": "https://www.qt.io"},
            ],
        },
        {
            "bom-ref": "ref-harfbuzz",
            "name": "harfbuzz",
            "version": "11.0.0",
            "licenses": [{"license": {"id": "MIT"}}],
            "externalReferences": [{"type": "website", "url": "https://harfbuzz.github.io"}],
        },
        {
            "bom-ref": "ref-dbus",
            "name": "dbus",
            "version": "1.15.8",
            "licenses": [
                {
                    "license": {
                        "expression": "(AFL-2.1 OR GPL-2.0-or-later)"
                        " AND DocumentRef-COPYING"
                    }
                }
            ],
        },
        {
            "bom-ref": "ref-gtest",
            "name": "gtest",
            "version": "1.16.0",
        },
        {
            "bom-ref": "ref-isolation",
            "name": "isolation",
            "version": "3.0",
            "licenses": [{"license": {"id": "MIT"}}],
        },
        {
            "bom-ref": "ref-nolicense",
            "name": "nolicense",
            "version": "1.0",
        },
        {
            "bom-ref": "ref-xorg",
            "name": "xorg",
            "version": "system",
            "licenses": [{"license": {"id": "MIT"}}],
        },
    ],
    "dependencies": [
        {"ref": "root-ref", "dependsOn": ["ref-qt"]},
        {"ref": "ref-qt", "dependsOn": ["ref-harfbuzz", "ref-dbus"]},
        {"ref": "ref-harfbuzz"},
    ],
}

DIRECT_REQUIRES = {"qt"}


def _manifest(tmp_path: Path):
    licenses_dir = tmp_path / "LICENSES"
    licenses_dir.mkdir()
    for lic in (
        "LGPL-3.0-only", "LicenseRef-Qt-Commercial",
        "GPL-2.0-or-later", "BSD-3-Clause",
    ):
        (licenses_dir / f"{lic}.txt").write_text("text")
    (licenses_dir / "MIT.txt").write_text("text")
    path = tmp_path / "dependencies.toml"
    path.write_text(MANIFEST_TEXT)
    return load_manifest(path, [licenses_dir]), licenses_dir


def test_builds_attribution_list(tmp_path):
    manifest, licenses_dir = _manifest(tmp_path)
    result = build_attributions(
        manifest, FRAGMENT, DIRECT_REQUIRES,
        zrythm_version="2.0.0", licenses_dirs=[licenses_dir],
    )
    by_id = {c["id"]: c for c in result["components"]}
    assert by_id["qt"]["version"] == "6.11.1"
    assert by_id["qt"]["via"] is None
    assert by_id["qt"]["homepage"] == "https://www.qt.io"
    assert by_id["qt"]["licenseFiles"] == [
        "LGPL-3.0-only.txt", "LicenseRef-Qt-Commercial.txt",
    ]
    assert by_id["rubberband"]["version"] == "4.0.0"
    assert "gtest" not in by_id
    harfbuzz = by_id["harfbuzz"]
    assert harfbuzz["via"] == "qt"
    assert harfbuzz["tier"] == "conan"
    assert harfbuzz["copyright"] is None
    assert harfbuzz["license"] == "MIT"
    assert harfbuzz["licenseFiles"] == ["MIT.txt"]


def test_system_packages_are_excluded(tmp_path):
    # Conan marks host-platform libraries with version "system"; they are not
    # distributed by us and must not appear in the attribution list.
    manifest, licenses_dir = _manifest(tmp_path)
    result = build_attributions(
        manifest, FRAGMENT, DIRECT_REQUIRES,
        zrythm_version="2.0.0", licenses_dirs=[licenses_dir],
    )
    by_id = {c["id"]: c for c in result["components"]}
    assert "xorg" not in by_id


def test_nested_expression_license_preserved(tmp_path):
    # Conan emits compound recipe licenses as {"license": {"expression": E}};
    # the raw fragment (unlike the merged SBOM) keeps that shape, and the
    # attribution generator must surface the expression string, not "Unknown".
    manifest, licenses_dir = _manifest(tmp_path)
    result = build_attributions(
        manifest, FRAGMENT, DIRECT_REQUIRES,
        zrythm_version="2.0.0", licenses_dirs=[licenses_dir],
    )
    by_id = {c["id"]: c for c in result["components"]}
    dbus = by_id["dbus"]
    assert dbus["via"] == "qt"
    assert dbus["license"] == (
        "(AFL-2.1 OR GPL-2.0-or-later) AND DocumentRef-COPYING"
    )
    assert dbus["licenseFiles"] == ["GPL-2.0-or-later.txt"]


def test_unreachable_component_via_is_unknown(tmp_path):
    # A fragment component that cannot be traced back to a direct require
    # is distinct from a direct dependency.
    manifest, licenses_dir = _manifest(tmp_path)
    result = build_attributions(
        manifest, FRAGMENT, DIRECT_REQUIRES,
        zrythm_version="2.0.0", licenses_dirs=[licenses_dir],
    )
    by_id = {c["id"]: c for c in result["components"]}
    assert by_id["isolation"]["via"] == "unknown"
    assert by_id["qt"]["via"] is None
    assert by_id["harfbuzz"]["via"] == "qt"


def test_unknown_license_yields_no_files_and_no_warning(tmp_path, capsys):
    manifest, licenses_dir = _manifest(tmp_path)
    result = build_attributions(
        manifest, FRAGMENT, DIRECT_REQUIRES,
        zrythm_version="2.0.0", licenses_dirs=[licenses_dir],
    )
    by_id = {c["id"]: c for c in result["components"]}
    assert by_id["nolicense"]["license"] == "Unknown"
    assert by_id["nolicense"]["licenseFiles"] == []
    assert "Unknown" not in capsys.readouterr().out


def test_missing_fragment_degrades(tmp_path):
    manifest, licenses_dir = _manifest(tmp_path)
    result = build_attributions(
        manifest, None, DIRECT_REQUIRES,
        zrythm_version="2.0.0", licenses_dirs=[licenses_dir],
    )
    by_id = {c["id"]: c for c in result["components"]}
    assert by_id["qt"]["version"] is None
    assert by_id["rubberband"]["version"] == "4.0.0"
