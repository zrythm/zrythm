<!--
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Zrythm Project Schemas

JSON Schema specifications for Zrythm file formats.

> [!WARNING]
> **UNSTABLE** - v2.alpha under active development

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
