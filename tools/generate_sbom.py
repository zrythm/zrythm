#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Generate sbom.cdx.json (CycloneDX 1.6).

Merges the Conan-tier fragment emitted during `conan install` (see
conanfile.py generate()) with the cpm/vendored/asset entries from
data/dependencies.toml. Validates the result against the CycloneDX schema.
Unlike the attribution generator, a requested fragment must exist: the
SBOM must be complete.
"""

import argparse
import json
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path

from dependency_manifest import Manifest, load_manifest

ROOT_BOM_REF_PREFIX = "pkg:gitlab/zrythm/zrythm@"


def _manifest_component(comp) -> dict:
    return {
        "bom-ref": f"zrythm:{comp.tier}:{comp.id}@{comp.version or 'unknown'}",
        "type": "file" if comp.tier == "asset" else "library",
        "name": comp.name,
        "version": comp.version or "unknown",
        "copyright": comp.copyright,
        "licenses": [{"expression": comp.spdx}],
        "externalReferences": [{"type": "website", "url": comp.homepage}],
    }


def _normalize_licenses(licenses: list) -> list:
    # Conan emits compound recipe licenses as {"license": {"expression": E}},
    # which the CycloneDX schema forbids: an expression must appear as
    # {"expression": E} directly. License ids/names keep their nested form.
    normalized = []
    for entry in licenses:
        lic = entry.get("license")
        if isinstance(lic, dict) and set(lic) == {"expression"}:
            normalized.append({"expression": lic["expression"]})
        else:
            normalized.append(entry)
    return normalized


def _normalize_fragment_component(comp: dict) -> dict:
    comp = dict(comp)
    if comp.get("licenses"):
        comp["licenses"] = _normalize_licenses(comp["licenses"])
    return comp


def build_sbom(
    manifest: Manifest, fragment: dict | None, zrythm_version: str
) -> dict:
    skipped = {c.id for c in manifest.components if not c.shipped}
    bom: dict
    if fragment is not None:
        bom = dict(fragment)
        bom["components"] = [
            _normalize_fragment_component(c)
            for c in fragment.get("components", [])
            if c.get("name") not in skipped
        ]
        bom["dependencies"] = list(fragment.get("dependencies", []))
        tools = fragment.get("metadata", {}).get("tools", {})
        fragment_root_ref = (
            fragment.get("metadata", {}).get("component", {}).get("bom-ref")
        )
    else:
        bom = {"components": [], "dependencies": []}
        fragment_root_ref = None
        tools = {
            "components": [
                {"type": "application", "name": "zrythm-sbom-generator"}
            ]
        }
    added_refs = []
    for comp in manifest.components:
        if comp.tier == "conan" or not comp.shipped:
            continue
        entry = _manifest_component(comp)
        bom["components"].append(entry)
        added_refs.append(entry["bom-ref"])
    root_ref = f"{ROOT_BOM_REF_PREFIX}{zrythm_version}"
    bom["components"].sort(key=lambda c: (c.get("name") or "").lower())
    bom["metadata"] = {
        "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "tools": tools,
        "component": {
            "bom-ref": root_ref,
            "type": "application",
            "name": "Zrythm",
            "version": zrythm_version,
            "licenses": [{"expression": "LicenseRef-ZrythmLicense"}],
            "externalReferences": [
                {"type": "website", "url": "https://zrythm.org"}
            ],
        },
    }
    conan_root_refs = [
        c["bom-ref"]
        for c in bom["components"]
        if c.get("purl", "").startswith("pkg:conan/")
    ]
    bom["dependencies"] = [
        d for d in bom["dependencies"] if d.get("ref") != root_ref
    ]
    for dep in bom["dependencies"]:
        depends = dep.get("dependsOn")
        if depends and fragment_root_ref is not None:
            dep["dependsOn"] = [
                r for r in depends if r != fragment_root_ref
            ]
    bom["dependencies"] = [
        d
        for d in bom["dependencies"]
        if d.get("ref") != fragment_root_ref
    ]
    bom["dependencies"].append(
        {"ref": root_ref, "dependsOn": sorted(conan_root_refs + added_refs)}
    )
    bom["serialNumber"] = f"urn:uuid:{uuid.uuid4()}"
    bom["bomFormat"] = "CycloneDX"
    bom["specVersion"] = "1.6"
    bom["version"] = 1
    return bom


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

    manifest = load_manifest(args.manifest, args.licenses_dirs)
    fragment = None
    if args.conan_fragment is not None:
        if not args.conan_fragment.exists():
            print(
                f"error: Conan fragment not found: {args.conan_fragment} "
                "(run conan install first, or omit --conan-fragment)"
            )
            return 1
        fragment = json.loads(args.conan_fragment.read_text())
    version = args.version_file.read_text().strip().removeprefix("v")
    bom = build_sbom(manifest, fragment, version)

    from cyclonedx.schema import SchemaVersion
    from cyclonedx.validation.json import JsonStrictValidator

    rendered = json.dumps(bom, indent=2)
    result = JsonStrictValidator(SchemaVersion.V1_6).validate_str(rendered)
    if result is not None:
        print(f"error: generated SBOM failed schema validation: {result}")
        return 1
    args.output.write_text(rendered + "\n")
    print(f"SBOM written to {args.output} ({len(bom['components'])} components)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
