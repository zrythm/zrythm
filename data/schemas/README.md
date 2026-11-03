<!--
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Zrythm JSON Schemas

JSON Schema specifications for Zrythm file formats and persisted UI state.

> [!WARNING]
> **UNSTABLE** - v2.alpha under active development

## common.schema.json

Definitions shared by all schemas (registry objects, tracks, arranger
objects, plugins, ports, parameters, etc.). Other schemas reference these via
external `$ref`s (`common.schema.json#/definitions/...`), which
`scripts/inline_json_schema_refs.py` inlines at build time to produce the
self-contained schemas that get embedded and used for validation.

## project.schema.json

Schema for Zrythm project files (`.zpj`).

### Document Structure

```json
{
  "documentType": "ZrythmProject",
  "formatMajor": 2,
  "formatMinor": 1,
  "appVersion": "2.0.0",
  "datetime": "2026-01-28T12:00:00Z",
  "title": "Project Name",
  "project": {
    "tempoMap": { ... },
    "transport": { ... },
    "tracklist": { ... },
    "registries": { ... }
  }
}
```

### Key Concepts

- **Registries**: UUID-indexed objects deserialized first, then referenced
- **Format versioning**: `formatMajor` for breaking changes, `formatMinor` for additions
- **Validation**: Embedded at compile-time, enforced on save and load

### Migration

When `formatMajor` changes:

1. Update the schema
2. Add migration logic in `ProjectJsonSerializer::deserialize()` to transform old → new
3. Test round-trip conversion (old → new → old)

See `doc/dev/project_serialization_flow.md` for serialization architecture.

## dock-layout.schema.json

Schema for the docking workspace state stored globally in `AppSettings`.

- `version` identifies the layout format.
- `main` contains the five workspace areas and their outer sizes. Areas hold
  tabbed frames, binary splits, or `null`.
- Split `sizes` contain two positive fractions, each at most 1;
  restoration normalizes them.
- `floating` contains detached workspaces, their geometry, and optional
  panel-to-frame return mappings. Floating windows are at least 200 by 140
  logical pixels.
- `hidden` lists registered panels excluded from all workspaces.

`DockingController.restoreState()` applies equivalent structural checks without
a runtime schema-validator dependency. Invalid documents leave the active
layout unchanged. Restoration then removes unknown panel references and
duplicate panel, frame, split, and window IDs, repairs stale active-panel
references, and reinserts missing registered panels.

Valid and invalid fixtures live under `tests/fixtures/schemas/dock-layout/`.
`scripts/validate_json_schemas.py` checks them together with the schema.

## clipboard.schema.json

Schema for Zrythm clipboard payloads (cut/copy/paste of arranger objects,
tracks and plugins). A payload is a self-contained snapshot mirroring the
project file's registry format: flat buckets with every object reachable
from the copied roots, the root UUIDs, and free-form per-type metadata. The
OS clipboard text form is the payload JSON, zstd-compressed and
base64-encoded, prefixed with `ZRYTHM-CLIPBOARD:v1:`.

### Document Structure

```json
{
  "documentType": "ZrythmClipboard",
  "formatVersion": 1,
  "payloadType": "arrangerObjects",
  "sourceProjectId": "5d1f7b4e-2a3c-4f8e-9b0d-1c2e3f4a5b6c",
  "metadata": { "anchorTicks": 2000.0 },
  "registry": { "ports": [], "parameters": [], "plugins": [],
                "tracks": [], "arrangerObjects": [], "fileAudioSources": [] },
  "roots": ["<uuid>"]
}
```
