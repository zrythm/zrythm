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

**IMPORTANT:** Always use `ctest --test-dir builddir_cmake` rather than `cd builddir_cmake && ctest`. The `--test-dir` flag allows running tests from any directory without changing the working directory.

```bash
# Run all tests via CTest
ctest --test-dir builddir_cmake --output-on-failure -j$(nproc)

# Run specific tests by pattern (uses regex matching on test names)
# Test names follow the pattern: TestClassName.TestMethodName
ctest --test-dir builddir_cmake -R "ProjectSerializationTest" --output-on-failure
ctest --test-dir builddir_cmake -R "ProjectLoaderTest" --output-on-failure
ctest --test-dir builddir_cmake -R "TransportControllerTest" --output-on-failure

# List all available tests to see test names
ctest --test-dir builddir_cmake -N

# Run specific test binary directly
./builddir_cmake/products/bin/zrythm_dsp_tempo_map_unit_tests

# Run specific test case with filter
./builddir_cmake/products/bin/zrythm_dsp_tempo_map_unit_tests --gtest_filter=TempoMapTest.testCaseName
```

### Packaging

For testing changes in packaged builds:

```bash
cpack -G AppImage -C Debug -B builddir_cmake
# Output: builddir_cmake/Zrythm-<version>-Linux.AppImage
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

---

## Project Overview

Zrythm is a highly automated and intuitive digital audio workstation (DAW) written in C++23 using Qt/QML and JUCE. It's free software tailored for both professionals and beginners.

**Key Technologies:**
- **Language**: C++23
- **UI Framework**: Qt6 (QML/QuickControls2)
- **Audio Framework**: JUCE 8

### Project Structure

```
zrythm/
├── src/                 # Main source code
│   ├── main.cpp        # Application entry point
│   ├── gui/            # Qt/QML user interface (backend/gtk_widgets and backend/backend/legacy_actions are legacy unused code kept for reference)
│   ├── dsp/            # Digital signal processing
│   ├── engine/         # Core audio engine
│   ├── plugins/        # Audio plugin hosting-related code
│   ├── structure/      # Project building blocks (tracks, objects, etc.)
│   └── utils/          # Utility functions
├── ext/                # Vendored dependencies
├── tests/              # Tests (only unit/, integration/ and benchmarks/ are active; other directories are legacy unused code kept for reference)
│   └── unit/           # Unit tests location
├── data/               # Application data (themes, samples, etc.)
└── i18n/               # Internationalization files
```

### Dependencies

Zrythm uses CPM (CMake Package Manager) for dependency management. Key dependencies include:
- **JUCE**: Audio framework
- **Qt6**: GUI framework (requires a recent version — see `CMakeLists.txt` for the minimum)
- **spdlog**: Logging
- **fmt**: String formatting
- **nlohmann_json**: JSON parsing
- **rubberband**: Audio time-stretching
- **googletest**: Testing framework
- **googlebenchmark**: Benchmarking framework
- **au**: Type-safe units

Dependencies are defined in [`package-lock.cmake`](package-lock.cmake) and fetched to `.cache/CPM/` (safe to delete, will re-fetch).

### Build System Notes

- **Binary Output**: `builddir_cmake/products/bin/`
- **Tests**: Enable with `-DZRYTHM_TESTS=ON`, `-DZRYTHM_BENCHMARKS=ON`, or `-DZRYTHM_GUI_TESTS=ON` during CMake configuration
- **Working Directory**: Never use `cd` for build/test commands; pass the build directory as an argument (e.g., `cmake --build builddir_cmake`, `ctest --test-dir builddir_cmake`)

---

## Git & CI

### Git Workflow

- **Repository**: This project uses a self-hosted GitLab instance at https://gitlab.zrythm.org/zrythm/zrythm
- All commits require sign-off: use `git commit -s` or add `Signed-off-by:` manually
- **Commit message style**: Use `<ClassName>: <imperative-summary>` format (e.g., `TrackCollection:`, `MoveTracksCommand:`, `TempoMap:`). If no single class is central to the change or too many classes are involved, use a general term related to what changed (e.g., `cmake:`, `tracks:`, `nlohmann-json:`). Follow with bullet points for significant details, keep summaries concise, and use backticks when referencing code in the body
- See [CONTRIBUTING.md](CONTRIBUTING.md) for DCO details
- Main branch: `master`, PR target: `master`
- Note: This branch is under major refactoring (see README.md warning)

### GitLab Interaction

Use the `glab` CLI tool to interact with the self-hosted GitLab instance, and use the full repository URL:

```bash
# CI/CD
# Note: use 'trace' instead of 'view' - 'view' requires a TTY
glab ci trace <job-id> -R https://gitlab.zrythm.org/zrythm/zrythm  # Get job logs (use | tail -N for large outputs)
glab ci list -R https://gitlab.zrythm.org/zrythm/zrythm            # List recent pipelines
glab ci get -b <branch> -R https://gitlab.zrythm.org/zrythm/zrythm # Get pipeline details (SHA, jobs) for a branch
glab ci get -p <pipeline-id> -R https://gitlab.zrythm.org/zrythm/zrythm  # Get pipeline details by ID

# Issues
glab issue view <id> -R https://gitlab.zrythm.org/zrythm/zrythm    # View issue details
glab issue close <id> -R https://gitlab.zrythm.org/zrythm/zrythm   # Close an issue
glab issue note <id> -R https://gitlab.zrythm.org/zrythm/zrythm -m "comment"  # Add comment to issue
glab issue update <id> -R https://gitlab.zrythm.org/zrythm/zrythm --label "label-name"  # Add label to issue
```

---

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

---

## Code Guidelines

### C++23

Zrythm makes extensive use of modern C++ features:
- Concepts and constraints
- Ranges and views

**C++ Code Guidelines:**
- Use standard algorithms (for example, `std::ranges::any_of`) instead of manual implementations
- Prefer `std::jthread` over `std::thread`
- Use `ptr == nullptr` instead of `!ptr` when doing null checks
- Use `std::numbers` instead of macros for number constants like `M_PI`
- Use ranges and range-based for-loops instead of C-style for-loops
- Utilize `std::views` where possible to make code more readable, for example for filtering, transforming, or even to simply loop n times using `std::views::iota`
- Avoid implicit conversions (`int` to `float`, `double` to `float`, etc.)
- Use `std::next` and `std::prev` instead of adding/subtracting to iterators directly
- Prefer `std::erase_if` over `std::remove_if` + `erase()`
- Avoid variable shadowing: use descriptive prefixes (e.g., `project_foo` instead of `foo`) when local variables would shadow class members
- Use west const style for simple const qualifiers (e.g., `const int x`, not `int const x`)
- Use `auto` for type-deduced variable declarations where the type is obvious from the initializer (e.g., `const auto &changes = tracker.changes();`, `auto * port = ...`)

### Unit Safety

**Strong Unit Type Usage Guidelines:**
- Prefer `au::max`, `au::min`, etc., over the `std` alternatives when the arguments are unit types

### Audio Processing

**DSP Code Guidelines:**
- Use SIMD-optimized float range operations from [here](src/utils/dsp.h) where possible
- Prefer real-time safe operations (avoid allocations, mutexes or other blocking code in audio thread hot paths)
- Use JUCE audio and MIDI buffer classes where it makes sense
- Implement proper thread safety

**Realtime Safety (`[[clang::nonblocking]]`):**
- Functions annotated with `[[clang::nonblocking]]` are real-time-safe contexts. When RealtimeSanitizer (RTSan) is enabled via `-fsanitize=realtime`, Clang treats calls within these functions as real-time context and flags any blocking operations (malloc, mutex locks, etc.)
- Audio processing functions (like `process_impl()`) and their callees must never allocate, lock mutexes, or call blocking APIs
- If a function is marked `[[clang::nonblocking]]`, ALL code paths within it must be non-blocking — this is enforced by RTSan at runtime

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

**QML Property Bindings:**
- Never use dead-expression tricks (e.g., `root.selectionModel.selection; // binding`) to create binding dependencies — they are unreliable in packaged builds where the QML engine may optimize away the statement
- Instead, use `Connections` with the appropriate signal handler (e.g., `onSelectionChanged`) to reactively update properties

### External Documentation

When you need to search external library/framework documentation (Qt, JUCE, etc.), use the `context7` tools (`resolve-library-id` then `query-docs`) rather than web search.

---

## Key Classes

### UUID-Identifiable Objects

See `UuidIdentifiableObject` in `src/utils/uuid_identifiable_object`.

### QObject-derived Objects

See [QObjectUniquePtr](src/utils/qt.h) for a unique pointer type for QObject-derived objects that takes into account parent ownership and does the right thing automatically. Prefer this over raw pointers when the object owner is the unique parent of the object.

### DSP Graph

See `graph.h`, `graph_node.h` under `src/dsp/`. [`EngineProcessTimeInfo`](src/dsp/graph_node.h) holds timing information that is passed to processing callbacks and is used throughout the code. [`ITransport`](src/dsp/itransport.h) abstracts some common transport functionality needed by the graph.

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

---

## Unit Tests

- Use gtest and gmock
- Use QTest for Qt utilities
- Unit tests go under `tests/unit/` with a structure corresponding to the source file path (example: `tests/unit/dsp/tempo_map_test.cpp`)
- Test filenames end in `_test.cpp`
- If a header is needed (to make qmoc happy for example when defining test QObjects), put it in `_test.h`
- Enclose the unit test classes and functions inside the namespace of the class being tested (avoid `using namespace`)

### Test Utilities

- [ScopedQCoreApplication](tests/helpers/scoped_qcoreapplication.h) (only needed when using QSignalSpy or other facilities that require an active Qt application)
- [ScopedJuceQApplication](tests/helpers/scoped_juce_qapplication.h): Inherits from ScopedQCoreApplication and also runs the JUCE message loop inside Qt's event loop. Only to be used when we can't avoid dependence on JUCE's message loop.
- [MockProcessable, MockTransport](tests/unit/dsp/graph_helpers.h)
- [MockTrack](tests/unit/structure/tracks/mock_track.h)
- Logging can be enabled in tests by calling `init_logging(LoggerType::Test)` (from `src/utils/logger.h`)

---

## Common Practices

- Always read the current state of a file before attempting changes
- Follow existing patterns for similar functionality when adding new features
- Ensure proper licensing headers (SPDX) on all new files
- Run formatting and linting before committing

---

## License and Copyright

- **Main License**: AGPL-3.0 with trademark use limitation
- **File Headers**: All files must include SPDX headers
- **Non-Code Files:** For files that cannot contain SPDX headers (JSON, schemas, images, etc.), add attribution in [`REUSE.toml`](REUSE.toml)

**Copyright Notice Format:**
```cpp
// SPDX-FileCopyrightText: © 2026 Your Name <your@email.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
```

---

*This document is maintained by the Zrythm development team. Last updated: 2026-04-21*
