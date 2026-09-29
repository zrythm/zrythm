# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from dependency_manifest import license_text_ids, load_manifest

REPO_ROOT = Path(__file__).resolve().parents[2]


def _write(tmp_path: Path, text: str, licenses: list[str]) -> Path:
    for i, group in enumerate((licenses[:1], licenses[1:])):
        licenses_dir = tmp_path / f"licenses-{i}"
        licenses_dir.mkdir()
        for lic in group:
            (licenses_dir / f"{lic}.txt").write_text("text")
    manifest = tmp_path / "dependencies.toml"
    manifest.write_text(text)
    return manifest


def _dirs(tmp_path: Path) -> list[Path]:
    return [tmp_path / "licenses-0", tmp_path / "licenses-1"]


def test_real_manifest_is_valid():
    manifest = load_manifest(
        REPO_ROOT / "data" / "dependencies.toml",
        [REPO_ROOT / "LICENSES", REPO_ROOT / "data" / "licenses"],
    )
    ids = {c.id for c in manifest.components}
    assert "qt" in ids and "juce" in ids and "rubberband" in ids
    assert len(manifest.components) > 50


def test_license_text_ids_parses_expressions():
    assert license_text_ids("MIT") == ["MIT"]
    assert license_text_ids("GPL-3.0-only OR LicenseRef-JUCE-Commercial") == [
        "GPL-3.0-only",
        "LicenseRef-JUCE-Commercial",
    ]
    assert license_text_ids("(Apache-2.0 AND MIT)") == ["Apache-2.0", "MIT"]
    # A WITH exception modifies a license instead of being a license of its
    # own, and DocumentRef-* points at a document inside the package: neither
    # has a license text to bundle.
    assert license_text_ids("GPL-2.0-or-later WITH Classpath-exception-2.0") == [
        "GPL-2.0-or-later",
    ]
    assert license_text_ids("DocumentRef-COPYING AND AFL-2.1") == ["AFL-2.1"]


def test_malformed_toml_raises_value_error(tmp_path):
    manifest = tmp_path / "dependencies.toml"
    manifest.write_text("[[component]\nid = ")
    with pytest.raises(ValueError, match="invalid TOML"):
        load_manifest(manifest, _dirs(tmp_path))


def test_missing_field_raises_value_error(tmp_path):
    manifest = tmp_path / "dependencies.toml"
    manifest.write_text('[[component]]\nname = "X"\n')
    with pytest.raises(ValueError, match="id"):
        load_manifest(manifest, _dirs(tmp_path))


def test_missing_license_text_rejected(tmp_path):
    manifest = _write(
        tmp_path,
        '[[component]]\nid="x"\nname="X"\ntier="asset"\nspdx="BSL-1.0"\n'
        'copyright="c"\nhomepage="https://x"\n',
        licenses=[],
    )
    with pytest.raises(ValueError, match="BSL-1.0"):
        load_manifest(manifest, _dirs(tmp_path))


def test_license_text_found_in_either_dir(tmp_path):
    manifest = _write(
        tmp_path,
        '[[component]]\nid="x"\nname="X"\ntier="asset"\nspdx="BSL-1.0 AND MIT"\n'
        'copyright="c"\nhomepage="https://x"\n',
        licenses=["MIT", "BSL-1.0"],
    )
    loaded = load_manifest(manifest, _dirs(tmp_path))
    assert [c.id for c in loaded.components] == ["x"]


def test_conan_tier_must_not_pin_version(tmp_path):
    manifest = _write(
        tmp_path,
        '[[component]]\nid="fmt"\nname="fmt"\ntier="conan"\nversion="12.0.0"\n'
        'spdx="MIT"\ncopyright="c"\nhomepage="https://x"\n',
        licenses=["MIT"],
    )
    with pytest.raises(ValueError, match="version"):
        load_manifest(manifest, _dirs(tmp_path))


def test_cpm_tier_requires_version(tmp_path):
    manifest = _write(
        tmp_path,
        '[[component]]\nid="clap"\nname="CLAP"\ntier="cpm"\nspdx="MIT"\n'
        'copyright="c"\nhomepage="https://x"\n',
        licenses=["MIT"],
    )
    with pytest.raises(ValueError, match="version"):
        load_manifest(manifest, _dirs(tmp_path))


def test_vendored_version_optional(tmp_path):
    manifest = _write(
        tmp_path,
        '[[component]]\nid="qm-dsp"\nname="QM DSP"\ntier="vendored"\n'
        'spdx="MIT"\ncopyright="c"\nhomepage="https://x"\n',
        licenses=["MIT"],
    )
    loaded = load_manifest(manifest, _dirs(tmp_path))
    assert loaded.components[0].version is None


def test_duplicate_ids_rejected(tmp_path):
    manifest = _write(
        tmp_path,
        '[[component]]\nid="z"\nname="Z"\ntier="asset"\nspdx="MIT"\n'
        'copyright="c"\nhomepage="https://x"\n'
        '[[component]]\nid="z"\nname="Z2"\ntier="asset"\nspdx="MIT"\n'
        'copyright="c"\nhomepage="https://x"\n',
        licenses=["MIT"],
    )
    with pytest.raises(ValueError, match="duplicate"):
        load_manifest(manifest, _dirs(tmp_path))


def test_invalid_tier_rejected(tmp_path):
    manifest = _write(
        tmp_path,
        '[[component]]\nid="z"\nname="Z"\ntier="submodule"\nspdx="MIT"\n'
        'copyright="c"\nhomepage="https://x"\n',
        licenses=["MIT"],
    )
    with pytest.raises(ValueError, match="tier"):
        load_manifest(manifest, _dirs(tmp_path))
