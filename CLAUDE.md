<!---
SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Zrythm - AI Agent Guide

This document provides essential coding guidelines for AI agents working with the Zrythm digital audio workstation project.

## Quick Reference

### Building

```bash
# Build (assumes builddir_cmake is already configured)
cmake --build builddir_cmake --config Debug

# Binary location: builddir_cmake/products/bin/zrythm
```

### Testing

```bash
# Run all tests via CTest
cd builddir_cmake && ctest --output-on-failure -j$(nproc)

# Run specific test binary directly
./builddir_cmake/products/bin/zrythm_dsp_tempo_map_unit_tests

# Run specific test case with filter
./builddir_cmake/products/bin/zrythm_dsp_tempo_map_unit_tests --gtest_filter=TempoMapTest.testCaseName
```

### Code Quality

```bash
# Format code
clang-format -i src/file.cpp

# Run pre-commit hooks
pre-commit run --all-files

# Run clang-tidy (requires compile_commands.json)
clang-tidy src/file.cpp -p builddir_cmake
```

### Git Workflow

- **Repository**: This project uses a self-hosted GitLab instance at https://gitlab.zrythm.org/zrythm/zrythm
- All commits require sign-off: use `git commit -s` or add `Signed-off-by:` manually
- **Commit message style**: Use `<scope>: <imperative-summary>` format (e.g., `refactor:`, `cmake:`, `nlohmann-json:`) followed by bullet points for significant details
- See [CONTRIBUTING.md](CONTRIBUTING.md) for DCO details
- Main branch: `master`, PR target: `master`
- Note: This branch is under major refactoring (see README.md warning)

### GitLab Interaction

Use the `glab` CLI tool to interact with the self-hosted GitLab instance, and use the full repository URL:

```bash
# CI/CD
glab ci trace <job-id> -R https://gitlab.zrythm.org/zrythm/zrythm
glab ci list -R https://gitlab.zrythm.org/zrythm/zrythm

# Issues
glab issue view <id> -R https://gitlab.zrythm.org/zrythm/zrythm
glab issue close <id> -R https://gitlab.zrythm.org/zrythm/zrythm
glab issue note <id> -R https://gitlab.zrythm.org/zrythm/zrythm -m "comment"
glab issue update <id> -R https://gitlab.zrythm.org/zrythm/zrythm --label "label-name"
```

---

## Project Overview

Zrythm is a highly automated and intuitive digital audio workstation (DAW) written in C++23 using Qt/QML and JUCE. It's free software tailored for both professionals and beginners.

**Key Technologies:**
- **Language**: C++23
- **UI Framework**: Qt6 (QML/QuickControls2)
- **Audio Framework**: JUCE 8

## Project Structure

```
zrythm/
├── src/                 # Main source code
│   ├── main.cpp        # Application entry point
│   ├── gui/            # Qt/QML user interface
│   ├── dsp/            # Digital signal processing
│   ├── engine/         # Core audio engine
│   ├── plugins/        # Audio plugin hosting-related code
│   ├── structure/      # Project building blocks (tracks, objects, etc.)
│   └── utils/          # Utility functions
├── ext/                # Vendored dependencies
├── tests/              # Tests
│   └── unit/           # Unit tests location
├── data/               # Application data (themes, samples, etc.)
└── i18n/               # Internationalization files
```

## Dependencies and Testing

### Key Dependencies

Zrythm uses CPM (CMake Package Manager) for dependency management. Key dependencies include:
- **JUCE**: Audio framework
- **Qt6**: GUI framework
- **spdlog**: Logging
- **fmt**: String formatting
- **nlohmann_json**: JSON parsing
- **rubberband**: Audio time-stretching
- **googletest**: Testing framework
- **googlebenchmark**: Benchmarking framework
- **au**: Type-safe units

Dependencies are defined in [`package-lock.cmake`](package-lock.cmake).

### Testing Framework

**Test Structure:**
- Unit tests located in [`tests/unit/`](tests/unit/)
- Benchmarks (when `ZRYTHM_BENCHMARKS=ON`)
- GUI tests (when `ZRYTHM_GUI_TESTS=ON`)

**Test Frameworks:**
- googletest for unit testing
- gmock for mocking
- googlebenchmark for performance benchmarks
- QTest for Qt utilities

## Architecture Documentation

Zrythm has comprehensive architecture documentation in the [`doc/dev/`](doc/dev/) directory. Key architectural systems include:

### Undo/Redo System
- **Location**: [`doc/dev/undo_system.md`](doc/dev/undo_system.md)
- **Purpose**: Provides robust undo/redo functionality with separation of concerns
- **Key Components**: Models, Factories, Commands, Actions, UndoStack
- **Pattern**: Model layer (pure data) → Commands (reversible mutations) → Actions (semantic operations) → UI

### Object Selection System
- **Location**: [`doc/dev/object_selection_system.md`](doc/dev/object_selection_system.md)
- **Purpose**: Unified selection system for arranger objects (regions, markers, notes, automation points)
- **Key Components**: UnifiedProxyModel, SelectionTracker, ArrangerObjectSelectionOperator
- **Integration**: Bridges C++ models with QML views through Qt's ItemSelectionModel

### Playback Cache Architecture
- **Location**: [`doc/dev/playback_cache_architecture.md`](doc/dev/playback_cache_architecture.md)
- **Purpose**: Thread-safe caching of track events for real-time playback
- **Key Components**: PlaybackCacheScheduler, MidiPlaybackCache, PlaybackCacheBuilder
- **Real-time Safety**: Uses farbot::RealtimeObject for atomic cache swapping

### Writing Documentation

When editing or creating [developer documentation](doc/dev/), focus on high level concepts, utilizing mermaid diagrams where possible, instead of concrete code. Only include actual code where you think it's appropriate.

### Additional Documentation

- [debugging.md](doc/dev/debugging.md): Debugging techniques and tips
- [repo-management.md](doc/dev/repo-management.md): Repository management guidelines
- [versioning.md](doc/dev/versioning.md): Versioning policies
- [arranger_object_positions.md](doc/dev/arranger_object_positions.md): Position handling
- [project_serialization_flow.md](doc/dev/project_serialization_flow.md): Project save/load format
- [scenes_architecture.md](doc/dev/scenes_architecture.md): Scene system design
- [plugin_group_architecture.md](doc/dev/plugin_group_architecture.md): Plugin grouping

## Key Code Patterns

### Build System

- **Qt Version**: Requires Qt 6.10.0 or later
- **Dependencies**: Fetched via CPM to `.cache/CPM/` (safe to delete, will re-fetch)
- **Binary Output**: `builddir_cmake/products/bin/`
- **Tests**: Enable with `-DZRYTHM_TESTS=ON` during CMake configuration

### C++23 Features

Zrythm makes extensive use of modern C++ features:
- Concepts and constraints
- Ranges and views

**C++ Code Guidelines:**
- Use standard algorithms (for example, `std::ranges::any_of`) instead of manual implementations
- Prefer `std::jthread` over `std::thread`
- Use `ptr == nullptr` instead of `!ptr` when doing null checks
- Use `std::numbers` instead of macros for number constants like `M_PI`
- Use ranges (including `std::views::iota`) instead of C-style for-loops
- Avoid implicit conversions (`int` to `float`, `double` to `float`, etc.)
- Use `std::next` and `std::prev` instead of adding/subtracting to iterators directly
- Prefer `std::erase_if` over `std::remove_if` + `erase()`

### Unit Safety

**Strong Unit Type Usage Guidelines:**
- Prefer `au::max`, `au::min`, etc., over the `std` alternatives when the arguments are unit types

### Audio Processing

**DSP Code Guidelines:**
- Use SIMD-optimized float range operations from [here](src/utils/dsp.h) where possible
- Prefer real-time safe operations (avoid allocations, mutexes or other blocking code in audio thread hot paths)
- Use JUCE audio and MIDI buffer classes where it makes sense
- Implement proper thread safety

### Qt/QML Integration

**GUI Development:**
- Use Qt6 QML for modern UI components
- Follow Qt coding conventions
- Use Qt's signal/slot system for event handling
- Implement proper model/view separation
- Use the following naming pattern for property declarations: `Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)`
- When connecting signals, use the overload that takes:
  1. The source object instance
  2. The source object signal
  3. The target object instance (as a context that lets Qt auto-remove this signal if the target is deleted - this is always required)
  4. The target object slot, or a lambda
- In QML JavaScript, prefer `let` and `const` over `var` when declaring variables if possible

## Key Classes

### UUID-Identifiable Objects

See `UuidIdentifiableObject` in `src/utils/uuid_identifiable_object`.

### QObject-derived Objects

See [QObjectUniquePtr](src/utils/qt.h) for a unique pointer type for QObject-derived objects that takes into account parent ownership and does the right thing automatically. Prefer this over raw pointers when the object owner is the unique parent of the object.

### DSP Graph

See `graph.h`, `graph_node.h` under `src/dsp/`. [`EngineProcessTimeInfo`](src/utils/types.h) holds timing information that is passed to processing callbacks and is used throughout the code. [`ITransport`](src/dsp/itransport.h) abstracts some common transport functionality needed by the graph.

### Audio Processors & Parameters

See `src/dsp/processor_base.h` for the base processor class and `src/dsp/passthrough_processors.h` for example implementation. See `src/dsp/parameter.h` for processor parameters.

### Tempo Map

The [tempo map](src/dsp/tempo_map.h) is responsible for mapping/converting timeline positions between samples, ticks and seconds.
We are using 960 PPQN.

See also `src/dsp/tempo_map_qml_adapter.h` for a QML wrapper of it.

### Arrangement

[ArrangerObject](src/structure/arrangement/arranger_object.h) is the base class of all arranger object types.

#### Looping Behavior

Some arranger objects are [loopable](src/structure/arrangement/loopable_object.h). Loopable objects always start playback from their "clip start position", then loop indefinitely at their "loop end position" back to the "loop start position" until the object's end position is reached.

### Tracks

[Track](src/structure/tracks/track.h) is the base class of all track types.

## Unit Tests

- Use gtest
- Use gmock where mocking is needed
- Unit tests go under `tests/unit/` with a structure corresponding to the source file path (example: `tests/unit/dsp/tempo_map_test.cpp`)
- Test filenames end in `_test.cpp`
- If a header is needed (to make qmoc happy for example when defining test QObjects), put it in `_test.h`
- Enclose the unit test classes and functions inside the namespace of the class being tested (avoid `using namespace`)

### Reusable Utilities

- [ScopedQCoreApplication](tests/helpers/scoped_qcoreapplication.h) (only needed when using QSignalSpy or other facilities that require an active Qt application)
- [ScopedJuceQApplication](tests/helpers/scoped_juce_qapplication.h): Inherits from ScopedQCoreApplication and also runs the JUCE message loop inside Qt's event loop. Only to be used when we can't avoid dependence on JUCE's message loop.
- [MockProcessable, MockTransport](tests/unit/dsp/graph_helpers.h)
- [MockTrack](tests/unit/structure/tracks/mock_track.h)

## Common Tasks for AI Agents

### Adding New Features

1. **Identify appropriate location** in the source tree
2. **Follow existing patterns** for similar functionality
3. **Add comprehensive tests** for new code
4. **Update documentation** if needed
5. **Ensure proper licensing** headers

### Modifying Existing Code

1. **Maintain API compatibility** when possible
2. **Update tests** to reflect changes
3. **Run formatting** before committing
4. **Verify build** on all supported platforms
5. **Always read the current state of a file** before attempting changes

### Bug Fixes

1. **Reproduce the issue** with minimal test case
2. **Add regression test** to prevent recurrence
3. **Document the fix** in code comments
4. **Commit with sign-off**: Use `git commit -s` to add DCO sign-off
5. **Consider backporting** to stable branches if applicable

## License and Copyright

- **Main License**: AGPL-3.0 with trademark use limitation
- **File Headers**: All files must include SPDX headers
- **Non-Code Files:** For files that cannot contain SPDX headers (JSON, schemas, images, etc.), add attribution in [`REUSE.toml`](REUSE.toml)

**Copyright Notice Format:**
```cpp
// SPDX-FileCopyrightText: © 2025 Your Name <your@email.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
```

---

*This document is maintained by the Zrythm development team. Last updated: 2026-01-23*
