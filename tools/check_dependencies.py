#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Fail if data/dependencies.toml drifts from the build system.

Run in CI (check stage) with no build required. Exit code 1 on drift.
"""

import sys
from pathlib import Path

from dependency_manifest import (
    Manifest,
    load_manifest,
    scan_conan_enabled_options,
    scan_conan_requirements,
    scan_cpm_declared_versions,
    scan_cpm_fetches,
    scan_vendored_dirs,
)

REPO_ROOT = Path(__file__).resolve().parent.parent
LICENSES_DIRS = [REPO_ROOT / "LICENSES", REPO_ROOT / "data" / "licenses"]


def find_drift(
    manifest: Manifest,
    scans: dict[str, set[str]],
    enabled_options: set[tuple[str, str]],
    declared_versions: dict[str, str],
) -> list[str]:
    """Return one message per mismatch between the manifest and the scans."""
    errors: list[str] = []
    by_id = manifest.by_id()
    for tier, names in scans.items():
        for name in sorted(names):
            comp = by_id.get(name)
            if comp is None:
                errors.append(f"{tier} dependency {name!r} has no manifest entry")
            elif comp.tier != tier:
                errors.append(
                    f"{name!r} is tier {comp.tier!r} in the manifest "
                    f"but was found in the {tier} scan"
                )
    for comp in manifest.components:
        if comp.tier == "asset":
            continue
        if comp.part_of is not None:
            # A part is not a Conan requirement of its own: conanfile.py must
            # enable the module option of the package that ships it.
            if (comp.part_of, comp.id) not in enabled_options:
                errors.append(
                    f"{comp.id!r} is part of {comp.part_of!r} but conanfile.py "
                    f'does not set self.options["{comp.part_of}"].{comp.id}'
                    f" = True"
                )
            continue
        if comp.id not in scans[comp.tier]:
            errors.append(
                f"manifest entry {comp.id!r} not found in the {comp.tier} scan "
                f"(stale entry?)"
            )
    for comp in manifest.components:
        if comp.tier == "cpm":
            expected = declared_versions.get(comp.id)
            if expected is None:
                errors.append(
                    f"{comp.id!r}: no VERSION declared in package-lock.cmake"
                )
            elif comp.version != expected:
                errors.append(
                    f"{comp.id!r}: manifest version {comp.version} != "
                    f"package-lock.cmake VERSION {expected}"
                )
    return errors


def main() -> int:
    try:
        manifest = load_manifest(
            REPO_ROOT / "data" / "dependencies.toml", LICENSES_DIRS
        )
    except ValueError as exc:
        print(f"error: invalid manifest: {exc}")
        return 1

    scans = {
        "conan": scan_conan_requirements(REPO_ROOT / "conanfile.py"),
        "cpm": scan_cpm_fetches(
            [REPO_ROOT / "CMakeLists.txt", REPO_ROOT / "ext" / "CMakeLists.txt"]
        ),
        "vendored": scan_vendored_dirs(REPO_ROOT / "ext"),
    }
    errors = find_drift(
        manifest,
        scans,
        scan_conan_enabled_options(REPO_ROOT / "conanfile.py"),
        scan_cpm_declared_versions(REPO_ROOT / "package-lock.cmake"),
    )
    for error in errors:
        print(f"error: {error}")
    if errors:
        return 1
    print(f"OK: {len(manifest.components)} components, no drift")
    return 0


if __name__ == "__main__":
    sys.exit(main())
