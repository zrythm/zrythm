# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Loader and validator for data/dependencies.toml.

The manifest is the single source of truth for shipped third-party
components. It backs both the CycloneDX SBOM (tools/generate_sbom.py) and
the in-app attribution display (tools/generate_attributions.py).

License texts live in two places: LICENSES/ holds texts for licenses used
by in-repo source files (maintained for REUSE compliance), and
data/licenses/ holds texts for licenses of external dependencies that no
in-repo file carries. Both directories are bundled into the application
resources.
"""

from __future__ import annotations

import ast
import re
import tomllib
from dataclasses import dataclass
from pathlib import Path

VALID_TIERS = ("conan", "cpm", "vendored", "asset")


@dataclass(frozen=True)
class Component:
    id: str
    name: str
    tier: str
    spdx: str
    copyright: str
    homepage: str
    shipped: bool = True
    version: str | None = None
    note: str | None = None


@dataclass(frozen=True)
class Manifest:
    components: tuple[Component, ...]

    def by_id(self) -> dict[str, Component]:
        return {c.id: c for c in self.components}


def license_text_ids(spdx: str) -> list[str]:
    """Return the SPDX ids in an expression that need a bundled text.

    WITH exceptions modify a license instead of being licenses of their
    own, and DocumentRef-* references point at documents inside a
    package; neither has a license text to bundle.
    """
    ids: list[str] = []
    skip_next = False
    for token in re.split(r"[\s()]+", spdx):
        if not token:
            continue
        if token == "WITH":
            skip_next = True
            continue
        if skip_next:
            skip_next = False
            continue
        if token in ("AND", "OR") or token.startswith("DocumentRef-"):
            continue
        ids.append(token)
    return ids


def load_manifest(path: Path, licenses_dirs: list[Path]) -> Manifest:
    """Parse and validate the dependency manifest.

    Raises ValueError with a descriptive message on any violation,
    including malformed TOML and missing fields.
    """
    try:
        data = tomllib.loads(path.read_text())
    except tomllib.TOMLDecodeError as exc:
        raise ValueError(f"invalid TOML: {exc}") from exc
    components = []
    seen_ids: set[str] = set()
    for raw in data.get("component", []):
        try:
            comp = Component(
                id=raw["id"],
                name=raw["name"],
                tier=raw["tier"],
                spdx=raw["spdx"],
                copyright=raw["copyright"],
                homepage=raw["homepage"],
                shipped=raw.get("shipped", True),
                version=raw.get("version"),
                note=raw.get("note"),
            )
        except KeyError as exc:
            raise ValueError(f"missing required field {exc}") from exc
        if comp.id in seen_ids:
            raise ValueError(f"duplicate component id: {comp.id}")
        seen_ids.add(comp.id)
        if comp.tier not in VALID_TIERS:
            raise ValueError(f"{comp.id}: invalid tier {comp.tier!r}")
        if comp.tier == "conan" and comp.version is not None:
            raise ValueError(
                f"{comp.id}: conan components must not pin a version"
            )
        if comp.tier == "cpm" and comp.version is None:
            raise ValueError(f"{comp.id}: version is required")
        for lic_id in license_text_ids(comp.spdx):
            if not any(
                (licenses_dir / f"{lic_id}.txt").exists()
                for licenses_dir in licenses_dirs
            ):
                raise ValueError(
                    f"{comp.id}: missing license text {lic_id}.txt in"
                    f" {', '.join(str(d) for d in licenses_dirs)}"
                )
        components.append(comp)
    return Manifest(components=tuple(components))


def scan_conan_requirements(conanfile_path: Path) -> set[str]:
    """Collect recipe names from self.requires()/tool_requires()/test_requires() calls.

    Only string-literal refs are collected; computed refs and python_requires
    are not supported.
    """
    tree = ast.parse(conanfile_path.read_text())
    names: set[str] = set()
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call):
            continue
        func = node.func
        if not (
            isinstance(func, ast.Attribute)
            and isinstance(func.value, ast.Name)
            and func.value.id == "self"
            and func.attr in ("requires", "tool_requires", "test_requires")
        ):
            continue
        if node.args and isinstance(node.args[0], ast.Constant):
            names.add(str(node.args[0].value).split("/")[0])
    return names


_CPM_FETCH_RE = re.compile(r"^\s*CPMGetPackage\(\s*([A-Za-z0-9_.-]+)")
_CPM_DECLARE_RE = re.compile(r"CPMDeclarePackage\(")
_CPM_FIELD_RE = {
    field: re.compile(rf"^\s*{field}\s+(\S+)", re.MULTILINE)
    for field in ("NAME", "VERSION")
}


def scan_cpm_fetches(cmake_paths: list[Path]) -> set[str]:
    names: set[str] = set()
    for path in cmake_paths:
        for line in path.read_text().splitlines():
            if line.lstrip().startswith("#"):
                continue
            match = _CPM_FETCH_RE.match(line)
            if match:
                names.add(match.group(1))
    return names


def scan_cpm_declared_versions(package_lock_path: Path) -> dict[str, str]:
    """Collect NAME → VERSION pairs from CPMDeclarePackage blocks.

    Blocks are delimited by the next CPMDeclarePackage call (or EOF)
    instead of by paren matching, so field values may contain ")".
    """
    text = package_lock_path.read_text()
    starts = [m.start() for m in _CPM_DECLARE_RE.finditer(text)]
    versions: dict[str, str] = {}
    for i, start in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else len(text)
        block = text[start:end]
        name = _CPM_FIELD_RE["NAME"].search(block)
        version = _CPM_FIELD_RE["VERSION"].search(block)
        if name and version:
            versions[name.group(1)] = version.group(1)
    return versions


def scan_vendored_dirs(ext_path: Path) -> set[str]:
    return {p.name for p in ext_path.iterdir() if p.is_dir()}
