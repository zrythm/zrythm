# Changelog
All notable changes to this project will be documented in this file.

## [0.6.038] - 2019-08-21
- Add MIDI out ports to channels and to JACK/ALSA
- Add JACK transport functionality (timebase master and client)
- Add scrollbar for ports list
- Add option to expose ports to JACK
- Add pre-fader/post-fader sends
- Add manpage
- Add compiler info to version string
- Add locked and enabled port connection attributes
- Show track ports in the inspector
- Move user manual from separate repository to this distribution
- Convert port connection knobs to sliders
- Check if port connections break acyclicity before connecting them
- Fix guitarix plugins crashing (add dependency to fftw3)
- Fix regression causing backward/forward button to not work properly
- Fix crash when deleting bus track with plugin
- Show dialog with backtrace when crashing
- Fix old frame positions getting used after BPM changes
- Fix ALSA output being silent
- Update German, Japanese, French translations

## [0.6.003] - 2019-08-13
- Plugin latency compensation during playback
- Chords and automation are now inside regions (can be looped)
- Refactor editor panel to share 4 different editors
- New chord editor
- New automation editor
- Change meter font and install it properly (should fix missing font in previous versions)
- Added some post-install commands to make sure Zrythm is installed properly
- Add region split functionality (WIP)
- Add quick quantize/full quantize functionality (regions only for now)
- Add metronome
- Update Chinese, German, French, Japanese translations
- Fader is now a separate processor in the signal chain
- Fix browser resize handle position not getting stored properly
- Fix incorrect loop range being shown in the editors
- Desktop file is now localized
- Add PACKAGING.md for packagers
- Fix plugin UI controls moving back and forth while changing them
- Fix crash when copying a region
- Fix pressing stop twice not moving the playhead to the cuepoint
- Fix many other bugs
- Add missing copyright/license notices in source files
- Refactoring, remove some unnecessary files

## [0.5.162] - 2019-07-14
- IMPORTANT: License change from GPLv3+ to AGPLv3+
- Add cut-clip tool (WIP)
- Add Norwegian in language selection
- Update German translations
- Update Norwegian translations
- Update Japanese translations
- Allow undoable individual MIDI velocity changes
- Add undoable MIDI velocity ramp feature
- Fix some undo/redo bugs
- Refactor of arranger objects
- Fix basic saving/loading of projects
- Fix duplicate projects getting displayed in the project list in some situations
- Refactor automation tracks

## [0.5.120] - 2019-06-29
- Change appearance of meters (added captions, made the text smaller)
- Change appearance of DSP/CPU meter (used icons insted of text)
- Allow undoable moving of arranger objects using the selection info bar
- German translations thanks to Waui (Weblate)
- Change appearance of plugin browser (using toggles instead of expanders)
- Add MIDI in activity bar in the top toolbar
- Allow filtering by plugin type (instrument/effect/modulator/midi effect) in the plugin browser
- Norwegian Bokmal translations thanks to kingu (Weblate)
- Remember plugin browser selections on each run

## [0.5.097] - 2019-06-21
- Drop windows support
- Add markers
- New design for chords/scales
- Change appearance of track icon (changes color to contrast track color)
- Write backtrace to log file when crashing
- Fix undo/redo issues when creating objects
- Highlight loop area
- Fix crash when creating automation points
- Automation curves now start as straight lines
- Make automation curves change slower when clicking and draging (better precision)
- Can now connect more than one modulator to a plugin parameter
- Can now set depth of modulation
- Fix some undo/redo bugs when duplicating objects
- Fix modulator size issue
- Add stack switcher in the inspector (track/editor/plugin properties)
- Can now move the playhead to the next/previous marker with numpad 4 and 6

## [0.5.074] - 2019-06-14
- Add Chord and Scale objects in the arranger
- Add Chord and Scale selector popups
- Add highlighting in the Piano Roll based on current Chord/Scale
- Create flatpak (WIP)
- Fix minor Timeline bugs

## [0.5.056] - 2019-06-10
- Fix some missing icons
- Update Japanese and French translations (thanks sub26nico and trebmuh)
- Add meson command to build dev docs
- Simplify MIDI note and region operations (internally)
- Add tests for midi notes and regions
- Make panels foldable by clicking on their tabs (#294)
- Add selection info bar for piano roll
- Add support for ALSA audio and MIDI (sequencer) backends
- Rewrite internal MIDI operations to be agnostic of the MIDI backend
- Fix issues with plugins crashing or not loading (due to missing compilation flags)
- Add live MIDI activity indicator to each track
- Fix MIDI note continuing to get played if moved while it's playing (WIP)
- Fix various minor bugs in the arrangers (ctrl+zoom, playhead z-position and others)


## [0.5.007] - 2019-06-04
- Switched from autotools to meson
- Fix all compiler warnings
- Fix snapping miscalculation where an off-grid selection would not snap to the grid but move by 1 snap point instead
- Fix some memory leaks
- Implement new system for showing arranger objects where each object holds pointers to its transients and lane counterparts
- Each track can now have multiple lanes
- Remove DISTRHO ports from tree
- Add additional pinned timeline above the main one
- Show icon of track inside its color box instead of next to it
- Add option to generate coverage reports
- Remove google fonts from docs
- Code cleanup
- Add Control Room functionality (WIP)
- Add Contributing Guidelines to the docs
- Add Modulator tab for automating plugin controls (WIP)
- Implement backward/forward button functionality

## [0.4.186] - 2019-05-23
- Update French translations
- Fix Windows build
- Remove libsmf from tree and import midilib
- Fix for Debian and other distors where tooltips were not showing and CPU meter was flickering
- Implement Drum Mode in the piano roll (works but WIP)

## [0.4.173] - 2019-05-21
- Fix windows build

## [0.4.170] - 2019-05-21
- Fix plugin getting removed from channel when moving the track
- New inspector UI (WIP)
- Can now connect ports from one plugin to another (CV to Control too)
- Change region appearance to show name inside dark background
- Added clang build to CI
- Show other regions in the piano roll (continuous view)
- New UI for developer docs (https://docs.zrythm.org)
- Use tabbed (notebook) toolbar instead of standard File/Edit/etc. #79
- Merge new French translations
- Add timeline selections info bar (WIP)
- Rearranged layout to make each sub-window self-contained
- Can now show/hide the timeline (fullscreen piano roll)

## [0.4.151] - 2019-05-12
- Fix FreeBSD build failures
- Fix crash on region duplication
- Fix duplicated MIDI notes not getting selected
- Fix piano roll not becoming active when clip editor is loaded
- Merge new libcyaml for bitfield support when saving projects
- Created expander box widget for use in the inspector/channel strip

## [0.4.139] - 2019-05-10
- Major changes in some internals of Zrythm (expect regressions)
- Most actions are now undoable
- Don't send UI updates to LV2 plugins until they get initialized (fixes Helm warnings)
- Plugins in the Mixer are now selectable and movable/duplicatable
- Can now route the output of channels to other channels
- Add more languages (Portuguese, Russian, Chinese)
- Update French translations thanks to sub26nico
- Channels/tracks are now movable/draggable (in the mixer only atm) - multiple channels can be moved at the same time
- Fix #242 compilation error on FreeBSD
- Made regions and midi notes prettier
- Fix numerous bugs
- Output logs to a file in zrythm main folder
- Multiple regions can now be resized simultaneously
- Minor bugfixes for MacOS

## [0.4.088] - 2019-04-29
- Defer deletion of some objects to give them time in case they are still used (fixes some warnings/algorithms)
- Fix all compiler warnings
- Use GAsyncQueue for event processing instead of custom event stack
- Fixed pan calculation unnecessarily being done multiple times per second (pan processing is much faster now)
- Implement sine law pan, square root pan and linear pan algorithms
- Option in preferences to choose the Pan Law and Pan Algorithm
- Clear buffers before sending them to the back end (this will probably prevent artifacts from appearing in some situations)
- Routing changes (such as adding a new plugin) no longer block the audio engine
- Fix crash when creating MIDI notes
- Use zix library from its repo instead of embedded in source code
- Fix/improve the auto-connect logic when routing in the mixer changes (including the logic for handling mono plugins)
- Plugins can now be moved to other slots (or channels) in the mixer (undoable)
- Creating plugins is now undoable

## [0.4.056] - 2019-04-24
- MacOS build fixes
- Fix CPU/DSP meters not refreshing regularly
- Added native window decoration
- New multi-thread processing algorithm thanks to Robin Gareus (DSP is very fast now)
- Show the initial window on top when running
- Implement PortAudio (WIP) and dummy backends
- Show CPU usage on mac
- Can now open plugins that showed a Mandatory port of unkown type error before
- Add option to build bundled plugins x42 and DISTRHO ports

## [0.4.027] - 2019-04-19
- MacOS support with .dmg installer
- Change audio engine processing algorithm to the one Ardour uses
- French translations
- Show CPU usage on Windows
- Minor cosmetic changes

## [0.4.003] - 2019-04-15
- Fix some midi note issues
- Make some libraries mandatory/optional depending on OS
- Windows support with installer

## [0.3.029] - 2019-04-08
- Fix random widget warnings for timeline arranger
- Fix #186 clang failing build for discarding qualifiers
- Fail on configure if libtool/gettext not found
- Zrythm now opens maximized
- Fix icon and window name for plugin windows
- Minor fixes
- Fix crashing/dangling events when closing plugin (tested with fabla)
- Fix deadlock when opening plugin UI during playback
- Use screen refresh rate as plugin UI refresh rate
- Fix #182 missing automation icons
- Portaudio is now optional (#177)
- Fix #173 deleting a track now destroys the plugins in that channel
- When a plugin is drag and dropped in a channel slot it now follows the setting to show ui on instantiate or not

## [0.3.013] - 2019-04-07
- Fix crash if no devices are connected to JACK
- Minor fixes and Japanese translations
- Fix numbering of regions/tracks
- Change labeling of regions/midi notes to not show debug info

## [0.3.006] - 2019-04-04
- Creating regions/midi notes is now undoable
- Fix multiple regions getting changed in edit mode when creating a region
- Architectural change on how tracks get selected
- Can now change track name and color in the inspector

## [0.3.000] - 2019-04-02
- Add donation dialog
- Fix CSS colors in dialogs/add missing
- Add shortcuts dialog
- Save as now defaults to the correct location
- Change the way the project is saved (objects are aggregated into separate data structures)

## [0.2.071] - 2019-03-31
- Add option to auto-connect selected MIDI controllers on startup
- Fix #154 piano roll not reattaching to correct track
- Fix #73 LV2 parameters trembling when changed in their UIs
- Add intro menu when Zrythm is first run

## [0.2.059] - 2019-03-29
- Fix crashing on saving project (after saving as once)
- Fix #150 adding new instrument track crashing
- Undo/redo technical improvement (revert objects to their original IDs)
- Fix inspector flickering while selecting objects
- Add undoable actions to timeline (ctrl + drag, move)
- Fix some undoable actions crashing in some cases

## [0.2.052] - 2019-03-28
- Made developer docs prettier (needs older version of doxygen)
- Added and broke auto-scroll for piano roll
- Localization
- Notes can be moved using arrow keys
- Notes duplicated with Ctrl+D (possibly broken)
- Optimization (not sleeping in glib idle functions)
- Added unit tests
- Added indicative colors and hover animation on channel slots
- Fixed icons in menu bar
- Added links in Help menu
- Changed the way moving notes works, can now Ctrl+drag to copy-move

## [0.2.003] - 2019-03-18
- Fix icons in menus
- Added links in Help menu

## [0.2.001] - 2019-03-17
- Fix multipaned ignoring resizing when starting dragging from leftright
- Fix automation tracklist lane IDs not getting saved
- Slight automatable optimization
- Fix automation point/curve widgets not visible after loading project
- Add DSP/CPU meter widget
- Cache automation curve drawing (optimization)
- Serialize IDs instead of objects in timeline selections
- Fix plugin UIs not opening/closing properly
- Add option to not auto-open plugin UIs
- Fix MIDI notes flickering when dragging past 127
- Fix ruler not showing range selection when selecting range from timeline
- Add some missing tooltips in instrument tracks

## [0.1.059] - 2019-03-17
- Ship breeze icons as compiled resources (fixed)

## [0.1.057] - 2019-03-17
- Ship breeze icons as compiled resources

## [0.1.055] - 2019-03-16
- added modes (select/edit/delete/ramp/audition)
- added delete functionality for midi notes with undo/redo
- fixed region delete crashing

## [0.1.046] - 2019-03-12
- added more missing tooltips
- added status bar that changes when hovering
- show current automation value in the automation lane

## [0.1.038] - 2019-03-10
- added tooltips for velocities/midi notes
- ctrl+L to loop selections
- fix piano roll notes staying pressed
- fix about dialog
- allow opening project while running

## [0.1.027] - 2019-03-09
- Debian 9 & Ubuntu 17 support

## [0.1.009] - 2019-03-05
- First release

----

Copyright (C) 2019 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.

