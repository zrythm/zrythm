#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Generate attributions.json for the in-app third-party license display.

Combines data/dependencies.toml with the Conan-tier fragment (for graph
versions and transitive components). Output is bundled into QRC at
configure time (see src/gui/CMakeLists.txt).

Unlike the SBOM generator, a missing fragment degrades gracefully (conan
versions stay null): the attribution display does not need to be complete
to be useful.
"""

import argparse
import json
import sys
from pathlib import Path

from dependency_manifest import (
    Manifest,
    license_text_ids,
    load_manifest,
    scan_conan_requirements,
)


def _license_files(spdx: str, licenses_dirs: list[Path]) -> list[str]:
    files = []
    for lic_id in license_text_ids(spdx):
        if any((d / f"{lic_id}.txt").exists() for d in licenses_dirs):
            files.append(f"{lic_id}.txt")
        else:
            print(f"warning: no license text for {lic_id}")
    return files


def _fragment_license(component: dict) -> str | None:
    parts = []
    for entry in component.get("licenses", []):
        if "expression" in entry:
            parts.append(entry["expression"])
        elif "license" in entry:
            lic = entry["license"]
            parts.append(
                lic.get("id") or lic.get("name") or lic.get("expression") or ""
            )
    return " OR ".join(p for p in parts if p) or None


def _website_url(refs: list) -> str | None:
    return next(
        (
            r["url"]
            for r in refs
            if r.get("type") == "website" and r.get("url")
        ),
        None,
    )


def _any_url(refs: list) -> str | None:
    return next((r["url"] for r in refs if r.get("url")), None)


def _via_map(fragment: dict, direct: set[str]) -> dict[str, str | None]:
    """Map each fragment bom-ref to the direct require it is pulled in via.

    None means the component is a direct require itself; "unknown" means
    it could not be traced back to any direct require.
    """
    by_ref = {c["bom-ref"]: c for c in fragment.get("components", [])}
    parents: dict[str, list[str]] = {}
    for dep in fragment.get("dependencies", []):
        for child in dep.get("dependsOn", []):
            parents.setdefault(child, []).append(dep["ref"])
    result: dict[str, str | None] = {}
    for ref, comp in by_ref.items():
        name = comp.get("name")
        if name in direct:
            result[ref] = None
            continue
        queue = list(parents.get(ref, []))
        via = None
        visited = set(queue)
        while queue:
            parent_ref = queue.pop(0)
            parent = by_ref.get(parent_ref)
            if parent is not None and parent.get("name") in direct:
                via = parent["name"]
                break
            for grandparent in parents.get(parent_ref, []):
                if grandparent not in visited:
                    visited.add(grandparent)
                    queue.append(grandparent)
        result[ref] = via if via is not None else "unknown"
    return result


def build_attributions(
    manifest: Manifest,
    fragment: dict | None,
    direct_conan_requires: set[str],
    zrythm_version: str,
    licenses_dirs: list[Path],
) -> dict:
    components = []
    fragment_by_name = {}
    if fragment is not None:
        fragment_by_name = {
            c.get("name"): c for c in fragment.get("components", [])
        }
    for comp in manifest.components:
        if not comp.shipped:
            continue
        entry = {
            "id": comp.id,
            "name": comp.name,
            "tier": comp.tier,
            "version": comp.version,
            "via": None,
            "license": comp.spdx,
            "copyright": comp.copyright,
            "homepage": comp.homepage,
            "licenseFiles": _license_files(comp.spdx, licenses_dirs),
        }
        if comp.tier == "conan":
            frag = fragment_by_name.get(comp.id)
            if frag is not None:
                entry["version"] = frag.get("version")
                website = _website_url(frag.get("externalReferences", []))
                if website is not None:
                    entry["homepage"] = website
        components.append(entry)
    if fragment is not None:
        skipped = {c.id for c in manifest.components if not c.shipped}
        manifest_ids = set(manifest.by_id())
        via = _via_map(fragment, direct_conan_requires)
        for frag_comp in fragment.get("components", []):
            name = frag_comp.get("name")
            if name in manifest_ids or name in skipped:
                continue
            if frag_comp.get("version") == "system":
                # Conan marks host-platform libraries (xorg, egl, opengl, ...)
                # as version "system": we do not distribute them, so they
                # need no attribution. The SBOM keeps them for graph
                # completeness.
                continue
            spdx = _fragment_license(frag_comp)
            refs = frag_comp.get("externalReferences", [])
            components.append({
                "id": name,
                "name": name,
                "tier": "conan",
                "version": frag_comp.get("version"),
                "via": via.get(frag_comp.get("bom-ref")),
                "license": spdx or "Unknown",
                "copyright": None,
                "homepage": _website_url(refs) or _any_url(refs),
                "licenseFiles": (
                    _license_files(spdx, licenses_dirs) if spdx else []
                ),
            })
    tier_order = {"conan": 0, "cpm": 1, "vendored": 2, "asset": 3}
    components.sort(
        key=lambda c: (tier_order.get(c["tier"], 9), c["name"].lower())
    )
    return {
        "application": {"name": "Zrythm", "version": zrythm_version},
        "components": components,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--conan-fragment", type=Path, default=None)
    parser.add_argument("--version-file", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--licenses-dirs",
        type=Path,
        nargs="+",
        default=[
            Path(__file__).resolve().parent.parent / "LICENSES",
            Path(__file__).resolve().parent.parent / "data" / "licenses",
        ],
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    manifest = load_manifest(args.manifest, args.licenses_dirs)
    fragment = None
    if args.conan_fragment is not None and args.conan_fragment.exists():
        fragment = json.loads(args.conan_fragment.read_text())
    else:
        print(
            "warning: no Conan fragment; conan-tier versions will be empty"
        )
    direct = scan_conan_requirements(repo_root / "conanfile.py")
    version = args.version_file.read_text().strip().removeprefix("v")
    result = build_attributions(
        manifest, fragment, direct, version, args.licenses_dirs
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(
        f"attributions written to {args.output} "
        f"({len(result['components'])} components)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
