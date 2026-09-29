# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from dependency_manifest import load_manifest
from generate_sbom import build_sbom

MANIFEST_TEXT = """
[[component]]
id = "fmt"
name = "fmt"
tier = "conan"
spdx = "MIT"
copyright = "Victor Zverovich"
homepage = "https://fmt.dev"

[[component]]
id = "gtest"
name = "GoogleTest"
tier = "conan"
spdx = "BSD-3-Clause"
copyright = "Google LLC"
homepage = "https://github.com/google/googletest"
shipped = false

[[component]]
id = "rubberband"
name = "Rubber Band Library"
tier = "vendored"
version = "4.0.0"
spdx = "GPL-2.0-or-later"
copyright = "Particular Programs Ltd"
homepage = "https://breakfastquay.com/rubberband/"

[[component]]
id = "inter-font"
name = "Inter font"
tier = "asset"
spdx = "OFL-1.1"
copyright = "The Inter Project Authors"
homepage = "https://rsms.me/inter/"
"""

FRAGMENT = {
    "bomFormat": "CycloneDX",
    "specVersion": "1.6",
    "version": 1,
    "serialNumber": "urn:uuid:11111111-1111-1111-1111-111111111111",
    "metadata": {
        "component": {
            "bom-ref": "special-root",
            "name": "zrythm/2.0.0",
            "type": "application",
        },
        "tools": {"components": [{"type": "application", "name": "Conan-io"}]},
    },
    "components": [
        {
            "bom-ref": "pkg:conan/fmt@12.1.0?rref=abc",
            "name": "fmt",
            "purl": "pkg:conan/fmt@12.1.0",
            "type": "library",
            "version": "12.1.0",
            "licenses": [{"license": {"id": "MIT"}}],
        },
        {
            "bom-ref": "pkg:conan/gtest@1.16.0?rref=def",
            "name": "gtest",
            "purl": "pkg:conan/gtest@1.16.0",
            "type": "library",
            "version": "1.16.0",
        },
        {
            "bom-ref": "pkg:conan/dbus@1.15.8?rref=ghi",
            "name": "dbus",
            "purl": "pkg:conan/dbus@1.15.8",
            "type": "library",
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
    ],
    "dependencies": [
        {"ref": "special-root", "dependsOn": ["pkg:conan/fmt@12.1.0?rref=abc"]},
        {"ref": "pkg:conan/fmt@12.1.0?rref=abc"},
    ],
}


def _manifest(tmp_path: Path):
    licenses_dir = tmp_path / "LICENSES"
    licenses_dir.mkdir()
    for lic in ("MIT", "BSD-3-Clause", "GPL-2.0-or-later", "OFL-1.1"):
        (licenses_dir / f"{lic}.txt").write_text("text")
    path = tmp_path / "dependencies.toml"
    path.write_text(MANIFEST_TEXT)
    return load_manifest(path, [licenses_dir])


def test_merges_fragment_and_manifest(tmp_path):
    bom = build_sbom(_manifest(tmp_path), FRAGMENT, zrythm_version="2.0.0")
    names = {c["name"] for c in bom["components"]}
    assert names == {"fmt", "dbus", "Rubber Band Library", "Inter font"}
    root = bom["metadata"]["component"]
    assert root["name"] == "Zrythm" and root["version"] == "2.0.0"
    root_dep = next(
        d for d in bom["dependencies"] if d["ref"] == root["bom-ref"]
    )
    assert len(root_dep["dependsOn"]) == 4
    rubber = next(c for c in bom["components"] if c["name"] == "Rubber Band Library")
    assert rubber["version"] == "4.0.0"
    assert rubber["licenses"] == [{"expression": "GPL-2.0-or-later"}]
    assert rubber["copyright"] == "Particular Programs Ltd"
    font = next(c for c in bom["components"] if c["name"] == "Inter font")
    assert font["type"] == "file"


def test_all_dependency_refs_resolve(tmp_path):
    # The fragment's synthetic root is replaced by the Zrythm root in
    # metadata; dependency entries pointing at the removed root must not
    # survive the merge.
    bom = build_sbom(_manifest(tmp_path), FRAGMENT, zrythm_version="2.0.0")
    refs = {c["bom-ref"] for c in bom["components"]}
    refs.add(bom["metadata"]["component"]["bom-ref"])
    for dep in bom["dependencies"]:
        assert dep["ref"] in refs, dep["ref"]
        for target in dep.get("dependsOn", []):
            assert target in refs, target


def test_works_without_fragment(tmp_path):
    bom = build_sbom(_manifest(tmp_path), None, zrythm_version="2.0.0")
    names = {c["name"] for c in bom["components"]}
    assert names == {"Rubber Band Library", "Inter font"}
    from cyclonedx.schema import SchemaVersion
    from cyclonedx.validation.json import JsonStrictValidator

    validator = JsonStrictValidator(SchemaVersion.V1_6)
    result = validator.validate_str(json.dumps(bom))
    assert result is None, result


def test_normalizes_nested_license_expressions(tmp_path):
    # Conan emits compound recipe licenses as {"license": {"expression": E}},
    # which the CycloneDX schema forbids (expression must not be nested
    # under "license"). The merge step must flatten them.
    bom = build_sbom(_manifest(tmp_path), FRAGMENT, zrythm_version="2.0.0")
    dbus = next(c for c in bom["components"] if c["name"] == "dbus")
    assert dbus["licenses"] == [
        {
            "expression": "(AFL-2.1 OR GPL-2.0-or-later)"
            " AND DocumentRef-COPYING"
        }
    ]


def test_output_passes_cyclonedx_validation(tmp_path):
    bom = build_sbom(_manifest(tmp_path), FRAGMENT, zrythm_version="2.0.0")
    from cyclonedx.schema import SchemaVersion
    from cyclonedx.validation.json import JsonStrictValidator

    validator = JsonStrictValidator(SchemaVersion.V1_6)
    result = validator.validate_str(json.dumps(bom))
    assert result is None, result
