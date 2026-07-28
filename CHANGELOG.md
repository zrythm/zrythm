<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou
SPDX-License-Identifier: FSFAP
-->

# Changelog
All notable changes to this project will be documented in this file.

For changes prior to v2.0.0, see [CHANGELOG-old.v1.md](CHANGELOG-old.v1.md).

## [v2.0.0-alpha.2] - 2026-07-29

### Added
- Chord pad panel with preset/scale support, polyphonic audition and
  undoable editing (#5169)
- Chord editor for placing and editing chord objects in progressions
  (#5169)
- Chord suggestion engine that recommends next chords based on the
  current chord and scale
- Chord symbols displayed inside chord clips on the timeline
- Chord track recording, including from chord pad notes (#5281)
- Musical mode: time-stretch audio clips to follow the project tempo
  (offline Rubberband), toggleable globally and per clip (#4368)
- Source BPM detection from audio file metadata, with automatic tempo
  estimation when metadata is missing (#4746)
- Pencil tool for creating objects; hold Ctrl and drag to paint
  consecutive objects (#5178)
- Eraser tool for deleting objects by clicking or dragging a rectangle
  (also via right-drag) (#5180)
- Cut tool for splitting objects at the clicked position (Alt+click)
  (#5179)
- Ramp tool for drawing linear velocity ramps across MIDI notes (#5181)
- Audition tool for playing from the clicked position while held
  (#5182)
- Editable base tempo and base time signature anchored at the start of
  the timeline
- BPM cell in the transport bar showing the tempo at the playhead,
  editable via an undoable dialog
- Tempo automation curve drawn behind tempo objects
- Draggable clip-start, loop-start and loop-end markers on the clip
  editor ruler
- Undo/Redo actions in the Edit menu showing the next action name
- Panel toggle buttons in the toolbar and View menu
- Generic parameter editor window for plugins without a native UI
- Open .zpj project files from file managers or the command line
  (#5275)
- MIDI note activity highlighting on piano roll keys
- Undoable velocity editing with per-note clamping and live value labels
- Alternating bar shading in the arranger background
- Language menu with per-language completion percentages and a "More
  Languages" submenu
- Optional Tracy profiler integration

### Changed
- Rename "regions" to "clips" throughout the UI and project format
  (#4368)
- Reimplement bundled Faust plugins as internal plugins instead of
  generated VST3/CLAP bundles
- Switch most dependency management from CPM to Conan
- Rebuild the live waveform, spectrum analyzer and meter displays on a
  new real-time-safe port observation system
- Rework automation rendering with a C++ canvas painter and
  segment-based curve evaluation on the audio thread
- Arranger drags and resizes now preview in place and commit as a
  single undo step on release
- Default to dark mode on startup
- Update translations (Spanish, Ukrainian, Chinese Simplified, German,
  Russian, French, Japanese and more)

### Fixed
- Faders, knobs and balance controls getting stuck when a parent
  scroll view takes over the drag (#5308)
- Crash from wrapped 32-bit MIDI timestamps after ~50 days of system
  uptime (Linux ALSA)
- Track name field keeping stale text and staying in edit mode when
  switching tracks (#5220)
- Deleting or cloning tempo and time signature objects failing
- Incorrect note-offs with overlapping same-pitch MIDI notes (#5279)
- Distorted automation curves when tempo changes occur within a clip
- Track selection pointing at the wrong track when tracks are hidden
- Selections of non-moved tracks being lost when moving tracks
- Stale entries for deleted plugin files shown in the plugin browser
- Failed plugin instantiation leaving broken tracks; an error dialog is
  now shown instead
- Load/save dialogs not defaulting to the configured new-project
  directory (#5211)
- Dock panels resizing smaller than their content, and the bottom dock
  being cut off on first load
- Peak meter and fader value labels shifting the layout as values
  change

### Removed
- Legacy GTK-era code (widgets, actions, event manager, gschema
  generator)
- Legacy real-time audio stretcher (superseded by the offline
  time-stretch engine)

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
