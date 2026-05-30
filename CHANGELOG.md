<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou
SPDX-License-Identifier: FSFAP
-->

# Changelog
All notable changes to this project will be documented in this file.

For changes prior to v2.0.0, see [CHANGELOG-old.v1.md](CHANGELOG-old.v1.md).

## [v2.0.0-alpha.1] - 2026-05-30

First official v2 alpha release. Zrythm v2 is a complete rewrite using Qt/QML
and JUCE, replacing the previous GTK-based UI.

### Added
- Qt/QML-based UI with hardware acceleration (Qt Quick)
- Plugin support via JUCE: VST3, LV2, LADSPA, AudioUnit
- Native plugin support: CLAP
- Tempo map with editable tempo and time signature objects
- ASIO support on Windows
- CPack packaging: AppImage (GNU/Linux), DragNDrop (macOS), InnoSetup (Windows)
- CI pipeline with Clang sanitizers, Valgrind, QML lint, ClangFormat checks
- Unit tests, integration tests, and benchmarks
- Use CPM for dependency management with version pinning

### Changed
- Migrated from GTK to Qt6/QML
- Migrated from Meson to CMake
- Migrated from C to C++23
- Bundled Faust plugins migrated from LV2 to JUCE/VST3

### Removed
- GTK, Carla, guile dependencies
- Meson build system
