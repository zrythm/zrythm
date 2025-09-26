<!---
SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Zrythm - AI Agent Guide

This document provides essential coding guidelines for AI agents working with the Zrythm digital audio workstation project.

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
- **Key Components**: UnifiedArrangerObjectsModel, SelectionTracker, ArrangerObjectSelectionOperator
- **Integration**: Bridges C++ models with QML views through Qt's ItemSelectionModel

### Playback Cache Architecture
- **Location**: [`doc/dev/playback_cache_architecture.md`](doc/dev/playback_cache_architecture.md)
- **Purpose**: Thread-safe caching of track events for real-time playback
- **Key Components**: PlaybackCacheScheduler, MidiPlaybackCache, PlaybackCacheBuilder
- **Real-time Safety**: Uses farbot::RealtimeObject for atomic cache swapping

## Key Code Patterns

### C++23 Features

Zrythm makes extensive use of modern C++ features:
- Concepts and constraints
- Ranges and views

### Audio Processing

**DSP Code Guidelines:**
- Use SIMD optimizations where possible
- Prefer real-time safe operations
- Use JUCE audio buffers and processing chains
- Implement proper thread safety

### Qt/QML Integration

**GUI Development:**
- Use Qt6 QML for modern UI components
- Follow Qt coding conventions
- Use Qt's signal/slot system for event handling
- Implement proper model/view separation

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

### Bug Fixes

1. **Reproduce the issue** with minimal test case
2. **Add regression test** to prevent recurrence
3. **Document the fix** in code comments
4. **Consider backporting** to stable branches if applicable

## License and Copyright

- **Main License**: AGPL-3.0 with trademark use limitation
- **File Headers**: All files must include SPDX headers

**Copyright Notice Format:**
```cpp
// SPDX-FileCopyrightText: © 2025 Your Name <your@email.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
```

---

*This document is maintained by the Zrythm development team. Last updated: 2025-09-26*
