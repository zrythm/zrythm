<!---
SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Zrythm - AI Agent Guide

This document provides essential coding guidelines for AI agents working with the Zrythm digital audio workstation project.

## Quick Reference

### Building

```bash
# Build (assumes conanbuild/Debug is already configured)
cmake --build conanbuild/Debug --config Debug

# Binary location: conanbuild/Debug/products/bin/zrythm
```

### Testing

**IMPORTANT:** Always use `ctest --test-dir conanbuild/Debug` rather than `cd conanbuild/Debug && ctest`. The `--test-dir` flag allows running tests from any directory without changing the working directory.

```bash
# Run all tests via CTest
ctest --test-dir conanbuild/Debug --output-on-failure -j$(nproc)

# Run specific tests by pattern (uses regex matching on test names)
# Test names follow the pattern: TestClassName.TestMethodName
ctest --test-dir conanbuild/Debug -R "ProjectSerializationTest" --output-on-failure
ctest --test-dir conanbuild/Debug -R "ProjectLoaderTest" --output-on-failure
ctest --test-dir conanbuild/Debug -R "TransportControllerTest" --output-on-failure

# List all available tests to see test names
ctest --test-dir conanbuild/Debug -N

# Run specific test binary directly (used when we want to run the full module test suite)
./conanbuild/Debug/products/bin/zrythm_dsp_unit_tests

# Run specific test case with filter
./conanbuild/Debug/products/bin/zrythm_dsp_unit_tests --gtest_filter=TempoMapTest.testCaseName
```

When investigating a test failure, run the failing test alone with `--output-on-failure` and **do not clip the output** — the full output (including ASan stack traces, assertion messages, and gtest logs) is often needed to diagnose the root cause. Use `-R "<TestName>"` to isolate a single test.

**Prefer batch-mode scripted GDB for debugging over adding temporary printfs.** Breakpoint command lists log values and auto-continue, giving you printf-style tracing without modifying code or rebuilding, and without an interactive session. Write a small script and run it with `gdb -batch -x`:

```gdb
# /tmp/trace.gdb
set pagination off
break some_file.cpp:64
commands
silent
printf "raw_state.size() = %d\n", raw_state.size()
continue
end
run
```

```bash
gdb -batch -x /tmp/trace.gdb --args ./conanbuild/Debug/products/bin/zrythm_integration_tests --gtest_filter=SomeTest
```

Tips: under ASan/LSan, set `ASAN_OPTIONS=detect_leaks=0 LSAN_OPTIONS=detect_leaks=0` (LSan aborts under ptrace). Method calls are often inlined in Debug builds and can't be evaluated in `printf`/`call` — break on each `return`/branch line instead to determine which path executes, use `dump binary memory <file> <start> <end>` to capture buffers, and use `catch throw` with `bt` to find where an exception originates. GDB also has `dprintf` (dynamic printf) for simple print-and-continue cases.

**All tests are assumed to pass at each commit.** If a test fails after uncommitted changes, it is almost always those changes' fault — do not waste time checking whether the test was already broken.

```bash
ctest --test-dir conanbuild/Debug -R "TestName" --output-on-failure
```

### Test Binary Target Names

Test binary targets follow a strict naming convention. **Do not guess target names** — derive them from the pattern:

- **Unit tests**: `zrythm_<path_with_underscores>_unit_tests` — where `<path_with_underscores>` is the directory path under `tests/unit/` with `/` replaced by `_` (e.g., `tests/unit/structure/tracks/` → `zrythm_structure_tracks_unit_tests`)
- **Benchmarks**: `zrythm_<path_with_underscores>_benchmarks` (same pattern, under `tests/benchmarks/`)
- **Integration tests**: `zrythm_integration_tests`

When unsure, list all targets with: `ctest --test-dir conanbuild/Debug -N`

### Packaging

For testing changes in packaged builds:

```bash
cpack -G AppImage -C Debug -B conanbuild/Debug
# Output: conanbuild/Debug/Zrythm-<version>-Linux.AppImage
```

### Code Quality

```bash
# Format code
clang-format -i src/file.cpp

# Run pre-commit hooks
pre-commit run --all-files

# Run clang-tidy (requires compile_commands.json)
clang-tidy src/file.cpp -p conanbuild/Debug
```

**Only run clang-format on C++ files** (`.cpp`/`.h`/`.hpp`). Never run it on QML, CMake, Python, or other non-C++ files — clang-format does not understand their syntax and mangles them beyond repair.

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
│   ├── gui/            # Qt/QML user interface (backend/gtk_widgets and backend/legacy_actions are legacy unused code kept for reference)
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

Dependencies follow a three-tier hierarchy — try the first tier before falling back to the next:

1. **Conan (primary)** — declared in [`conanfile.py`](conanfile.py) (`requirements()` / `build_requirements()` / `test_requires()`), generated into CMake via `CMakeDeps`, and consumed with `find_package(...)`. This covers most deps: `qt`, `fmt`, `spdlog`, `nlohmann_json`, `libsndfile`, `boost`, `au`, `onetbb`, `tracy`, `gtest`, `benchmark`, etc. The lockfile is [`conan.lock`](conan.lock); profiles live in [`conan/profiles/`](conan/profiles).
2. **CPM (fallback for what Conan doesn't package)** — declared in [`package-lock.cmake`](package-lock.cmake) via `CPMDeclarePackage(...)` (pin a `GIT_TAG`; pass cache entries via `OPTIONS`; usually `SYSTEM YES` + `EXCLUDE_FROM_ALL YES`), then fetched explicitly with `CPMGetPackage(<name>)` in the root `CMakeLists.txt`. Note `CPMAddPackage` is never used. Currently only `clap`, `clap-helpers`, `vst3sdk`, and `farbot` go through CPM; the package-lock also mirrors several Conan deps as documentation, but they never fetch because Conan's `find_package` resolves first. Packages are cached under `.cache/CPM/` (safe to delete, will re-fetch).
3. **Full vendoring ([`ext/`](ext/))** — third-party trees copied in-tree (e.g. `soxr`, `rubberband`, `kissfft`, `zita-resampler`, `qm-dsp`), wired via `add_subdirectory(ext)`. Before adding a dep to Conan or CPM, check whether it (or an equivalent) is already vendored here. Vendored trees are excluded from clang-format (`.clang-format-ignore`) and have their licensing declared in [`REUSE.toml`](REUSE.toml).

To add a dependency: add it to `conanfile.py` and consume via `find_package`; if it's not available in Conan, add a `CPMDeclarePackage` block to [`package-lock.cmake`](package-lock.cmake) plus a matching `CPMGetPackage(<name>)` in `CMakeLists.txt`; only vendor into `ext/` as a last resort.

### Build System Notes

- **Binary Output**: `conanbuild/Debug/products/bin/`
- **Tests**: Enable with `-DZRYTHM_TESTS=ON`, `-DZRYTHM_BENCHMARKS=ON`, or `-DZRYTHM_QML_TESTS=ON` during CMake configuration
- **Working Directory**: Never use `cd` for build/test commands; pass the build directory as an argument (e.g., `cmake --build conanbuild/Debug`, `ctest --test-dir conanbuild/Debug`)
- **AUTOMOC cache staleness**: When adding a new header containing `Q_OBJECT`/`Q_GADGET`/`QML_ELEMENT`, CMake's AUTOMOC may fail to pick it up even after reconfiguring, producing linker errors like `undefined symbol: ...::staticMetaObject` or `vtable for ...`. Fix by deleting the affected target's autogen directories and reconfiguring. For the GUI library:
  ```bash
  rm -rf conanbuild/Debug/src/zrythm_gui_lib_autogen \
         conanbuild/Debug/src/CMakeFiles/zrythm_gui_lib.dir/zrythm_gui_lib_autogen
  cmake --preset default
  ```
  The same pattern applies to other targets — substitute the target name in the paths.

---

## Git & CI

### Git Workflow

- **Repository**: This project uses a self-hosted GitLab instance at https://gitlab.zrythm.org/zrythm/zrythm
- Commit messages follow [doc/dev/commit_messages.md](doc/dev/commit_messages.md): subject and body style, the required `Signed-off-by:` DCO sign-off (`git commit -s`), and trailers (`Fixes #N`, `Implements #N`, `GitLab-Work-Item: #N`)
- See [CONTRIBUTING.md](CONTRIBUTING.md) for DCO details
- See [AI_POLICY.md](doc/dev/AI_POLICY.md) for `Assisted-by:` trailers, which must be included in commit messages
- Main branch: `master`, PR target: `master`
- Note: This branch is under major refactoring (see README.md warning)
- **Release notes / change summaries:** When summarizing changes between releases or writing release notes/announcement posts, read [CHANGELOG.md](CHANGELOG.md) first — it is the curated, grouped source of truth. Use `git log` only to fill in gaps, since the raw log includes noise (translation merges, formatting, etc.)

### GitLab Interaction

Use the `glab` CLI tool to interact with the self-hosted GitLab instance, and use the full repository URL:

```bash
# CI/CD
# Note: use 'trace' instead of 'view' - 'view' requires a TTY
# IMPORTANT: job traces are re-downloaded on every fetch. When analyzing a trace,
# save it to a local file ONCE (e.g. `glab ci trace <job-id> ... > /tmp/trace.log`)
# and grep/read that file afterwards - do not re-run trace for each search.
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

### General

- **Never use the Write tool on existing files.** Always use the Edit tool to make targeted changes, preserving all existing comments, blank lines, and formatting. The Write tool may only be used for new files that don't exist yet.
- **Never assume our own code is immutable.** This is our codebase — we can and should modify any part of it (including base classes, utility types, build config, existing APIs) when it leads to a better design. When a constraint in existing code blocks the ideal solution, propose changing the code rather than working around it. Do not rationalize why a workaround is necessary; offer to fix the root cause instead.
- **Fail fast, noisily, and at the source.** Validate external input (plugin callbacks, file data, user actions) at the boundary where it enters our code, and reject invalid conditions with a   visible error (warning log + refusal of the operation, or an assertion for contract violations). Do not clamp, coerce, or otherwise silently "fix" bad values deeper in the system — that masks bugs and makes them surface far from their cause.
- Use assertions for catching programmer bugs, and exceptions (or error/warning logs + refusals/early returns) for catching unsupported runtime input
- **Prefer the cleanest correct approach over quick fixes.** Long-term good design and maintainability always outweigh temporary refactoring costs — take the time to refactor properly rather than layering workarounds on top of a flawed design.

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
- Use `std::span` instead of array pointers and sizes; for read-only accessors returning a collection, return `std::span<const T>` (a lightweight view) rather than `const std::vector<T>&`, so callers can't mutate the contents and the interface isn't tied to a specific container type
- Utilize `std::views` where possible to make code more readable, for example for filtering, transforming, or even to simply loop n times using `std::views::iota`
- Use `_` as the name for unused loop variables (e.g., `for (const auto _ : std::views::iota (0, n))`)
- Avoid implicit conversions (`int` to `float`, `double` to `float`, etc.)
- Prefer `std::ranges::next` and `std::ranges::prev` over the legacy `std::next`/`std::prev` (and over adding/subtracting to iterators directly) — the `std::ranges::` versions work correctly on range-adaptor iterators where the legacy ones don't
- Prefer `std::erase_if` over `std::remove_if` + `erase()`
- Avoid variable shadowing: use descriptive prefixes (e.g., `project_foo` instead of `foo`) when local variables would shadow class members
- Use west const style for simple const qualifiers (e.g., `const int x`, not `int const x`)
- Use `auto` for type-deduced variable declarations where the type is obvious from the initializer (e.g., `const auto &changes = tracker.changes();`, `auto * port = ...`)
- Prefer pimpl (pointer to implementation) for non-trivial class members that don't need to be exposed in the header, to reduce include dependencies and improve compile times
- API doc comments (Doxygen `@brief`, `@param`, `@return`) must describe the contract — what the function does, its parameters, return value, preconditions, and edge cases — not who calls it or why it was introduced. Mentioning specific callers (e.g. "exposed for use by X") couples the docs to internal architecture and goes stale when those callers change; design rationale belongs in commit messages or architecture docs, not the method's API comment
- When a class derives the same template twice (e.g. `TempoObjectManager` derives both `ArrangerObjectOwner<TempoObject>` and `ArrangerObjectOwner<TimeSignatureObject>`), member lookup is ambiguous. Disambiguate with explicit base-class scope resolution (e.g. `manager->structure::arrangement::ArrangerObjectOwner<...TempoObject>::get_sorted_children_view()`), not `static_cast`
- **Comments describe what, not why-not**: In code comments, state what the code or test does objectively. Do not narrate bug history, explain what a previous version did wrong, describe what must "not" happen, or reference specific third-party products as bug archaeology — commit messages and architecture docs are the right place for those; state durable constraints objectively instead (e.g., "latencyGet() is only allowed once activate() returns"). Test names follow the same rule: name them after the contract being verified, in a way that still makes sense when the motivating bug is long forgotten
- Avoid jargon in code comments and test names where possible/reasonable — use plain, direct language (e.g., "X must fire when Y, or Z goes stale" instead of "the change relay is armed")

### Unit Safety

**Strong Unit Type Usage Guidelines:**
- Use strong types (`units::sample_t`, `units::precise_tick_t`, etc.) everywhere — including struct members, function parameters, and local variables. Never extract to raw `int64_t`/`double` when the operation can be done directly on the strong type
- Prefer `au` math functions (`abs`, `min`, `max`, `clamp`, etc.) over the `std` alternatives when the arguments are unit types. Use **unqualified** calls (e.g., `abs(q)`, not `au::abs(q)` or `std::abs(q.in(units::samples))`) — see [au math docs](https://aurora-opensource.github.io/au/main/reference/math/)
- Never write patterns like `std::abs(a.source_frame.in(units::samples) - a.output_frame.in(units::samples)) <= tol.in(units::samples)` — instead write `abs(a.source_frame - a.output_frame) <= tol`

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
- APIs intended for real-time (audio thread) use must be marked `noexcept [[clang::nonblocking]]` on the declaration

### Qt/QML Integration

**GUI Development:**
- Use Qt6 QML for modern UI components
- Follow Qt coding conventions
- Use Qt's signal/slot system for event handling
- Implement proper model/view separation
- Use the following naming pattern for property declarations: `Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)`
- Q_PROPERTY types must use fully qualified class names (e.g., `zrythm::dsp::ProcessorParameter *`, not `ProcessorParameter *`) — even for types declared in the same namespace or included via headers. Note: this only applies to Q_PROPERTY, not to Q_INVOKABLE (which can use local type aliases or unqualified names)
- When connecting signals, use the overload that takes:
  1. The source object instance
  2. The source object signal
  3. The target object instance (as a context that lets Qt auto-remove this signal if the target is deleted - this is always required)
  4. The target object slot, or a lambda
- In QML JavaScript, prefer `let` and `const` over `var` when declaring variables if possible

**QML Property Bindings:**
- Never use dead-expression tricks (e.g., `root.selectionModel.selection; // binding`) to create binding dependencies — they are unreliable in packaged builds where the QML engine may optimize away the statement
- Instead, use `Connections` with the appropriate signal handler (e.g., `onSelectionChanged`) to reactively update properties
- Never use `parent.parent.someProperty` chains to access delegate properties from child items — they are fragile and break easily. Use IDs instead (e.g., give the delegate an `id: myDelegate` and reference `myDelegate.someProperty`)

### UI Design

- We mostly follow [Apple's Human Interface Guidelines](https://developer.apple.com/design/human-interface-guidelines/) for UI/UX decisions
- Use title-style capitalization in menus (e.g., "Show Experimental Translations", not "Show experimental translations")

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
- For bugfixes, write a failing repro test first (TDD) when the bug is deterministically reproducible through contract-level APIs; do not force it when reproduction would require destabilizing the test process (e.g., resource exhaustion) or adding production-code seams solely for testability — inspection and review suffice there
- Unit tests go under `tests/unit/` with a structure corresponding to the source file path (example: `tests/unit/dsp/tempo_map_test.cpp`)
- Test filenames end in `_test.cpp`
- If a header is needed (to make qmoc happy for example when defining test QObjects), put it in `_test.h`
- Enclose the unit test classes and functions inside the namespace of the class being tested (avoid `using namespace`)

### Flaky Test Prevention

- Use condition variables, latches, `QSignalSpy::wait()`, or `QTest::qWaitFor()` for synchronization — never `sleep()` or timed waits such as `QTest::qWait()`
- Make temp directories unique per test (e.g., prepend test name) and clean them up in teardown
- Avoid depending on real-time clocks, network, or filesystem state in unit tests; mock or abstract these away
- Use deterministic seeds for any randomized test inputs (e.g., pass a fixed seed to `std::mt19937`)

### Test Utilities

- [ScopedQCoreApplication](tests/helpers/scoped_qcoreapplication.h) (only needed when using QSignalSpy or other facilities that require an active Qt application)
- [ScopedJuceQApplication](tests/helpers/scoped_juce_qapplication.h): Inherits from ScopedQCoreApplication and also runs the JUCE message loop inside Qt's event loop. Only to be used when we can't avoid dependence on JUCE's message loop.
- [MockProcessable, MockTransport](tests/unit/dsp/graph_helpers.h)
- [MockTrack](tests/unit/structure/tracks/mock_track.h)
- Logging can be enabled in tests by calling `init_logging(utils::LoggerType::Test)` (`#include "utils/logger.h"`)

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

*This document is maintained by the Zrythm development team. Last updated: 2026-07-23*
