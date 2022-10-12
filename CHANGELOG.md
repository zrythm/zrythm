<!---
SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou
SPDX-License-Identifier: FSFAP
-->

# Changelog
All notable changes to this project will be documented in this file.

## [1.0.0-beta.3.8.17] - 2022-10-12
### Changed
- Update Korean, Spanish, French, Indonesian, Catalan, Portuguese translations
- Project assistant: rename Open Recent to Open Project
- Bump required carla and libpanel versions

### Fixed
- Fix "check for updates?" dialog not being modal

### Removed
- Drop rtkit code (replace with GDBus - currently disabled)

## [1.0.0-beta.3.8.1] - 2022-10-11
### Added
- Add fade in/out when muting/unmuting tracks (fixes pops)
- Allow moving chord objects vertically

### Changed
- Add more error checking when writing audio to file
- Optimize bar slider drawing (fixes lags in plugin inspector)

### Fixed
- Fix cue marker being invisible
- Fix pressing Home not setting the cue marker
- Fix plugin and file drag-n-drop being broken on Windows
- Fix track icon selector being empty

## [1.0.0-beta.3.7.1] - 2022-10-10
### Added
- JSFX and CLAP plugin support

## [1.0.0-beta.3.6.1] - 2022-10-03
### Added
- Jump to marker position when clicked

### Changed
- Add "Scales" indicator text in the bottom of the chord track
- Make spacebar toggle play/pause regardless of the focused widget (with exception for text editor widgets)

### Fixed
- Fix hang when exporting audio when project contains VST3 plugins
- Fix markers incorrectly being shown when marker track is invisible
- Fix incorrectly allowing to clone/rename start/end markers

## [1.0.0-beta.3.5.1] - 2022-10-01
### Added
- Switch to new file chooser widget (from GNOME Builder)
- Add option to ghost MIDI notes from other regions in the same track
- Add fade in/out when audio engine starts/stops to prevent clicks

### Changed
- Update Chinese (Simplified), Indonesian, Catalan, Spanish, Ukrainian, German, Vietnamese translations
- Update bundled weakjack
- Make some debug output silent in normal builds
- Handle failure to connect JACK monitor output gracefully
- Add more system info to bug report templates (`DESKTOP_SESSION` and `XDG_CURRENT_DESKTOP`)
- Update libpanel dependency to >= 1.0.0
- Meson: change fft3 detection (added new options `fftw3_threads_separate`, `fftw3_threads_separate_type` and `fftw3f_separate`)

### Fixed
- Fix incorrectly showing bug report dialog when alpha project upgrade fails
- Fix newlines not showing in the release notes in the About dialog
- Fix error when selecting an invalid WAV file in the file browser
- Fix incorrectly allowing to delete/clone the master track when inside a foldable track
- Fix attempting to autosave when main window not set up yet
- Fix error when clicking on a chord region then an audio region
- Fix error when cutting a region while snap-keep-offset is enabled

## [1.0.0-beta.3.4.1] - 2022-09-02
### Added
- Write FINISHED file after finishing saving a project and check for this file on load
- Use new higher quality "Finer" timestretcher when using rubberband v3

### Changed
- Update French, Japanese translations
- Change trial version limitation to max 25 tracks per project

### Fixed
- Fix header widget not unparenting its child stack

## [1.0.0-beta.3.3.3] - 2022-09-01
### Added
- Add track filtering by name and type
- Add more error checking when instantiating Carla plugins

### Changed
- Don't allow moving the playhead while recording
- Don't show red peak on MIDI meters
- Windows MME: only print debug messages if `ZRYTHM_DEBUG` is set
- Update Spanish, Russian, French, Catalan, Chinese (Simplified), Chinese (Traditional) translations
- Stop SIGSEGV handler from being called multiple times
- Support `LC_ALL` from the environment
- Move track visibility toggles to new track filtering popover
- Avoid showing bug report dialogs for known issues that are not Zrythm bugs

### Fixed
- Fix error when splitting a large audio clip
- Fix screen moving backwards when zooming in towards the end of the project
- Fix autoplay breaking when changing directories in the file browser
- Fix file info label not being updated when changing directories in the file browser
- Fix error when right-clicking on Macro knob
- Fix direct connection port selector incorrectly allowing connecting audio ports to CV ports
- Fix some memory leaks
- Fix incorrectly calling `g_log_set_writer_func()` more than once causing an error on exit on some systems
- Fix automation mode switches in automation tracks being ellipsized on some systems

## [1.0.0-beta.3.2.1] - 2022-08-21
### Added
- Allow clicking anywhere on timeline minimap to navigate
- Add sorting by name, last used and most used in plugin browser
- User manual: add note about using non-flatpak plugins in flatpak builds
- Add right click option on tracks to show used and hide unused automation lanes
- Allow double-clicking on automation points to specify exact value
- Allow renaming tracks via right click menu
- Add right click option to create an object on all arrangers

### Changed
- Update Spanish, French, Catalan, Chinese (Traditional) translations
- Use case-insensitive alphabetical sorting in plugin browser
- Use vst instead of vstlx for flatpak VST2 plugin scan paths
- Make MIDI in label in header untranslatable to avoid sizing issues

### Fixed
- Fix chord and scale selector windows not closing with Esc key
- Fix plugin info label not being updated when filtering
- Fix transport display and ruler right click menus not working

## [1.0.0-beta.3.1.1] - 2022-08-12
### Changed
- Drop vendored zix and depend on zix library
- Replace some icons with icons from GNOME icon library
- Replace about dialog with AdwDialogWindow

### Fixed
- Meson: fix lilv-related subprojects
- Fix bug report format breaking when undo history is empty
- Fix VST3 support being disabled

## [1.0.0-beta.3.0.17] - 2022-08-09
### Changed
- Update Chinese (Simplified), Chinese (Traditional), Catalan, French translations
- Meson: use official lilv repository as subproject
- User manual: explain automation lane on/off buttons

### Fixed
- Fix lag when clicking on plugins in a large project
- Fix long loading time when loading large projects
- Fix crash when zooming in on audio regions

## [1.0.0-beta.3.0.1] - 2022-08-04
### Changed
- Update French, Chinese (Simplified), Catalan, Turkish, Portuguese (Brazil), Japanese, Polish, Hebrew, Indonesian, Ukrainian, Russian, Portuguese, Italian, German, Thai translations
- Add better error handling when failing to read project yaml
- Ignore GTK critical message (widget size related)
- Meson: accept libpanel alpha release version
- Upgrade project format and drop undo history when loading older projects

### Fixed
- Fix error when lowering BPM and saving a project with audio regions
- Fix loading projects with audio regions under different sample rate
- Fix endless stream of bug report dialogs (only allow a single dialog to be open at any time)
- Fix error when adding an insert to an audio track
- Fix error when attempting to change buffer size
- Fix error when attempting to bounce a track while transport is playing

## [1.0.0-beta.2.1.1] - 2022-04-29
### Added
- Use libpanel for docking/panels
- Add search entry in file browser
- Add AIFF, AU, CAF, W64 export support

### Changed
- Redesign export dialog
- Update French, Chinese (Simplified), Catalan translations

### Fixed
- Fix error when creating a region in the 3rd automation track
- Fix undo history getting cleared when deleting tracks

## [1.0.0-beta.2.0.3] - 2022-04-23
### Added
- Add more bundled plugins: Flanger, Phaser, Wah4, Triple Synth, Parametric EQ, Highpass Filter, Lowpass Filter, Peak Limiter, White Noise
- Implement add track button on tracklist and mixer
- Add more metadata in bundled plugins
- Enable vectorization optimizations on bundled plugins
- Extract more control port info from Carla plugins

### Changed
- Change port symbols for some bundled plugins
- Avoid attempting to get too many backtraces at once
- Preferences: convert icon and CSS theme selection entries to dropdowns

### Fixed
- Fix paths not being updated when selected from a file chooser button
- Fix positioning of context menu on BPM and playhead position widgets
- Fix saving a backup removing files from the main project's audio pool instead of the backup's

## [1.0.0-beta.1.5.1] - 2022-04-14
### Added
- Add status page to modulators tab
- Add bundled plugins: compressor, delay, gate, distortion, reverb

### Changed
- Add some vertical padding to automation editor
- Port some split buttons to AdwSplitButton
- Simplify check for unsaved changes (only look at last performed action)
- Update French, Chinese (Simplified), Portuguese, Catalan translations
- User manual: update some sections
- User manual: change PDF author to 'The Zrythm contributors'
- Hardcode standard LV2 paths for all OSes
- Update screenshot URL in metainfo

### Fixed
- Fix preference rows not being centered
- Fix arranger objects not being draggable from their bottom/left side
- Various CSS/style fixes
- Fix issues with fonts in some custom widgets

## [1.0.0-beta.1.4.1] - 2022-04-10
### Changed
- Become a full libadwaita app
- Simplify theme CSS

### Fixed
- Fix defaulting to JACK on Windows/Mac
- Fix compilation failure on some distros
- Fix crash when passing --reset-to-factory
- Fix fonts disappearing on Windows after opening preferences

### Removed
- Remove matcha theme

## [1.0.0-beta.1.3.1] - 2022-04-07
### Added
- Include system info in automatic bug reports
- Add clang-format target
- Implement base/full MIDI export
- Preferences: add option to reset to factory settings
- Add ECMAScript support for scripting
- Add language selection in scripting dialog

### Changed
- Switch to REUSE v3.0 and SPDX license identifiers
- Reformat whole source code using clang-format
- User manual: update initial configuration, scripting sections
- Simplify initial configuration: only select language and path
- Update German, French, Japanese, Chinese (Simplified), Turkish translations
- Update README, INSTALL, HACKING content
- Redesign bug report dialog
- Preferences: use subtitles instead of tooltips
- Welcome dialog: point to trademark policy
- Welcome dialog: mention that only flatpak-packaged plugins are supported in flatpak builds

### Fixed
- Fix formatting in appdata XML
- User manual: fix broken references
- Fix MIDI region content selection in MIDI export dialog
- Fix some memory leaks reported by gcc sanitizer
- Fix error when right-clicking inside audio editor

### Removed
- Remove unused bootstrap js files

## [1.0.0-beta.1.2.3] - 2022-03-29
### Fixed
- Fix PDF manual build

## [1.0.0-beta.1.2.1] - 2022-03-29
### Added
- Use one instance for each channel for mono plugins
- Include release changelog info in appstream data

### Changed
- Scroll to mid note when first showing the piano roll
- Update Hungarian translations
- INSTALL: clarify build instructions
- Enable locale detection for all languages
- TRADEMARKS: simplify some wording and reserve right to review and object to use deemed outside the policy
- Make sure UI event queue is cleared before freeing (fixes occasional errors)
- User manual: update Routing, Chords and Scales, Modulators, Scripting, Theming, User Media, Contributing and Credits chapters
- User manual: update some URLs (fix permanent redirects)

### Fixed
- Fix automation lanes not being shown immediately when made visible
- Fix MIDI files not being activatable in the file browser
- Fix audio engine not being resumed after running Guile scripts
- Fix engine preprocessing sometimes running while the graph is being updated
- Fix various invalid accesses reported by GCC address sanitizer
- Fix various memory leaks reported by GCC leak sanitizer
- Fix various custom widget children not being unparented during dispose
- Fix segfault when attempting to add a plugin to a collection
- Fix scale objects throwing errors in the event viewer
- Fix plugin sidechain options not being available in channel sends

### Removed
- Remove clang-tidy targets from meson configuration (speeds up reconfiguration)
- User manual: remove copyright and license info from translatables
- Remove drop motion handler from timeline (should fix errors on drag and hover)
- Remove emails from AUTHORS/THANKS/TRANSLATORS

## [1.0.0-beta.1.1.11] - 2022-03-23
### Changed
- User manual: update Editing, Mixing, Playback & Recording chapters
- Show warning when attempting to load unsupported projects
- Ignore CRITICAL message when opening native file chooser in Flatpak builds
- Make editor toolbar scrollable

### Fixed
- Fix opening projects from latest backup missing plugin states

## [1.0.0-beta.1.1.1] - 2022-03-19
### Added
- Add Hungarian locale

### Changed
- User manual: update Getting Started, Interface, Configuration, Projects, Plugins & Files, Tracks chapter
- Update GTK-related subproject versions
- Disable scroll-to-focus on arrangers
- Cleanup plugin state dirs on save

### Fixed
- Fix applying audio function not updating clip frames
- Fix MIDI export adding silence between regions
- Fix non-fatal error when importing empty MIDI files

### Removed
- Remove message that says Zrythm is in alpha

## [1.0.0-beta.1.0.1] - 2022-03-16
### Added
- Add MIDI format selector in the export dialog
- Add option to export track lanes as separate MIDI tracks in the export dialog
- Cancel current arranger action on Escape press

### Changed
- Update copyright years in about dialog
- Resize MIDI and velocity arrangers proportionally when resizing the editor
- Queue some startup messages to be shown after main window loads
- Improve context menu styling
- Clear undo history when deleting channel slots or tracks with uninstantiated plugins
- Use custom-built test instrument instead of Geonkick in some tests
- Update default screenshot in appdata

### Fixed
- Fix plugin state dirs in backups being empty
- Fix audio FX track stems being silent
- Fix being unable to open main window on MacOS

### Removed
- Remove warranty disclaimer from welcome dialog (already mentioned in about dialog)

## [1.0.0-alpha.30.3.1] - 2022-03-13
### Added
- Force an app icon in the header bar
- Auto-reconnect to any hardware devices that get disconnected
- Add some missing internal port symbols for channels/tracks

### Changed
- Move zoom buttons to timeline and editor toolbars
- Move about button to Help toolbar
- Make Edit icon lighter
- MIDI CC recording must now be enabled manually via automation lanes (improves DSP performance of MIDI and instrument tracks)
- Optimize plugin DSP processing
- Consider region as looped if loop end point is beyond region end
- Re-enable PipeWire support for Flatpak (add related message in welcome dialog)
- Make accelerators in popover menus orange
- Group various button groups together in toolbars

### Fixed
- Fix plugin passthrough ignoring MIDI events
- Fix incorrect track routing when using MIDI FX plugins
- Fix error when splitting unlooped automation regions
- Fix hardware inputs not being routed properly to tracks
- Fix "all audio inputs" in track inputs not behaving as intended
- Fix incorrect logic when checking whether a note is in a scale

## [1.0.0-alpha.30.2.1] - 2022-03-10
### Added
- Show project title/path in header bar
- Implement record on MIDI input
- Show toast message when loading/saving presets
- Add standard preset names for presets without names

### Changed
- Switch to GtkHeaderBar as client side decoration
- Make port selector a dialog instead of popover
- Clear monitor output when returning early from engine processing
- Lock port operation semaphore before changing transport states

### Fixed
- Fix crash when attempting to connect modulator outputs
- Fix right clicking after creating an object creating another object
- Fix plugin preset list not being cleared when inspecting a new plugin
- Fix errors when saving carla plugin presets
- Fix project assistant crash when no recent projects exist

### Removed
- Remove project version compatibility warning

## [1.0.0-alpha.30.1.1] - 2022-03-08
### Added
- Add option to create pre-routed setup for multi-out instruments
- Add new project loading dialog
- Allow opening a project from a path in the project loading dialog
- Allow renaming markers with F2
- Show version in splash screen
- Add Shift-Space shortcut to start transport in record mode
- Add default velocity selector with option to use last edited velocity

### Changed
- Consider region as looped if clip start position is not 0
- Expand track name in track properties
- Apply auto-scrolling when playhead is moved manually
- Update welcome dialog
- Update main screenshot in appdata

### Fixed
- Fix crash when resizing audio regions with custom clip start points
- Fix error when loading projects in different sample rates
- Fix track regions being moved incorrectly when moving tracks
- Fix port connection row displaying current port instead of connected port
- Fix various memory leaks
- Fix buffer overflow when operating on large numbers of objects

### Removed
- Remove reduntant checks during audio region processing

## [1.0.0-alpha.30.0.1] - 2022-03-05
### Added
- Implement track lane mute/solo
- Include whether X11 or Wayland in automatic bug reports
- Allow recording in chord track
- Allow playing chords with a MIDI keyboard
- Allow exporting multiple MIDI regions to MIDI file
- Allow exporting MIDI lanes as separate tracks in MIDI files

### Changed
- Move some MIDI track/lane context menu items under submenus
- Update German, Catalan, Korean, Indonesian translations

### Fixed
- Fix occasional error when drag-n-dropping in tracklist
- Fix chord track not being bounced
- Fix bounce dialog being empty and throwing errors
- Fix some MIDI track/lane context menu items not working
- Fix MIDI region export
- Fix compilation failure on some systems (missing `assert()`)

## [1.0.0-alpha.29.1.1] - 2022-02-23
### Added
- Add status page for "no clip selected" in the editor
- Add 10 samples of builtin fade in/out on all audio regions
- Add automation/chord region stretching

### Changed
- Add developer name to appdata XML
- Update Catalan, Japanese, Chinese (Simplified), Turkish, French translations
- Port event viewers to GtkColumnView
- Use separate event viewer for each editor
- Make all arranger objects editable in event viewers
- Require Ctrl modifier for global keypad 4 and 6 shortcuts

### Fixed
- Add missing type="desktop" to appdata XML
- Fix appdata XML having 2 default screenshots
- Do not allow stretching when a selected region is looped
- Fix clicking and dragging on bottom right of regions doing stretch instead of loop in stretch mode

### Removed
- Remove duplicate arranger object position validator

## [1.0.0-alpha.29.0.7] - 2022-02-13
### Changed
- Generate appdata XML using Guile script

## [1.0.0-alpha.29.0.1] - 2022-02-12
### Added
- Add more scales
- User manual: add more info about DSSI/LADSPA and Flatpak plugin paths
- Chord inversions
- Chord presets and chord preset packs
- Chord preset auditioning
- Ability to transpose entire chord pad
- Add meson options for optional dependencies
- Make chord changes undoable

### Changed
- Change how scales/chords are handled
- Use adaptive grid snap by default
- Hardcode Flatpak plugin paths
- Make region icon sizes smaller
- Print chord names in chord regions in timeline
- Use GtkCenterBox for bottom bar (makes Zrythm usable on small screen sizes)
- Select listview items on right click
- Port file auditioner instrument dropdown to GtkDropDown
- Unset `GTK_THEME` on startup (only support Zrythm themes)
- Update Catalan, French, Chinese (Simplified), Portuguese, German translations
- Do not block buttons below toast widget
- Do not allow setting loop end marker at or before loop start marker
- Do not allow cloning unclonable tracks

### Fixed
- Fix various issues with chord selector window
- Fix occasional stuck notes when clicking chords
- Fix error when pasting when clipboard is empty
- Fix error when showing port connections on input port
- Fix incorrect MIDI channel being sent when auditioning MIDI notes

## [1.0.0-alpha.28.1.3] - 2022-01-22
### Added
- Audition mode
- Adaptive snap
- Sidechain port detection for plugins running through Carla

### Changed
- Update French, Chinese (Simplified), Portuguese, Turkish, German, Italian translations
- Port MIDI activity widget to snapshot API
- Change CPU usage widget color
- Calculate nearest snap point on the spot instead of using pre-cached snap points
- Use custom pthreads with RT scheduling and low stack size for DSP on all backends
- Don't block when running various dialogs
- Carla: use timeout instead of tick callback for running UIs
- Carla: clear GTK GL context before opening UIs
- `zrythm_launch`: Ignore `LD_LIBRARY_PATH` and `GSETTINGS_SCHEMA_DIR` exports when using flatpak

### Fixed
- Silence GTK DND error on X11 (known GTK bug)
- Fix delete key not working in arrangers
- Fix Carla port connection issue with CV variants

## [1.0.0-alpha.28.0.1] - 2022-01-19
### Added
- Add appdata
- Optimize MIDI event processing
- Port various MIDI util functions from Tracktion
- Add tint to track and channel widgets
- Cache automation region drawing in timeline
- Optimize arranger drawing

### Changed
- Update Polish, Portuguese, Turkish, Chinese (Traditional), Chinese (Simplified), French, Hebrew, Indonesian translations
- Update fader and panner appearance
- Port track widget drawing to snapshot API
- Use theme color for playhead
- Use theme colors for meter gradient
- Change default track colors to palette from GTK color picker
- Make grid lines less prominent in the arranger
- Use port symbols for all ports (fixes projects made in one locale showing errors in another locale)
- Add Space, 1, 2, 3, 4, 5, 6 and other single-key shortcuts to arranger widget and remove some unnecessary global shortcuts
- Use most appropriate (slimmest) Carla patchbay variant for each plugin

### Fixed
- Fix "please restart" dialog being transient for preferences window instead of main window
- Fix snap to grid button not being togglable
- Fix `zrythm_launch` on MacOS (use `DYLD_LIBRARY_PATH`)
- Fix loop dashed line being incorrectly drawn at the start of regions
- Fix spacing of plugin/file browser filter buttons
- Add exception for bug report dialog for known GTK bug
- Fix non-fatal error when opening folded tracks
- Fix drop target not being set properly on folder channel widget
- Fix incorrectly setting temporary font config as global current
- Fix editable label popover not having its text selected
- Fix audio send buffers not being cleared during processing

### Removed
- Stop using trademarked MIDI logo

## [1.0.0-alpha.27.0.3] - 2022-01-10
### Added
- New dependency libadwaita
- New searchable preference dialog
- Add plugin latency handling for plugins running through Carla
- New Hebrew translation
- Various drawing optimizations
- Various DSP optimizations

### Changed
- Updated plugin browser filter section
- Change UI toolkit from GTK3 to GTK4
- Bump version requirements for some dependencies
- Use `LD_LIBRARY_PATH` to override library paths in `zrythm_launch`
- Run all plugins via Carla
- Port most widget drawing from cairo (software rendering) to GTK snapshot API (OpenGL)
- Show toast messages when backups are saved instead of showing a blocking popup
- Use SCSS to compile CSS theme
- Allow DSEG font loading directly from file
- Update Indonesian, Ukrainian, Greek, Italian, Spanish, Slovenian, French, Turkish, Portuguese (Brazil), Portuguese, Chinese (Simplified), Russian translations
- Use libdir option for Zrythm lib directory instead of 'lib'
- Port some widgets to new GTK4 alternatives
- Use meson dictionary to generate list of languages
- Use Carla patchbay variant instead of rack for loading plugins to support CV ports
- Change global single-key shortcuts to require Ctrl modifier
- Make strict compilation flags stricter
- Add additional gtksourceview5 language spec lookup path
- Redraw rulers and arrangers on every frame
- Recalculate DSP graph when reallocating engine ports
- Use `int_fast64_t` and `uint_fast64_t` for large DSP numbers instead of `long` for better cross-platform compatibility
- Set Carla plugin window parent on Windows so the plugin window stays on top of Zrythm
- Do not attempt to run `diff` to check for changes when closing the project

### Fixed
- Fix issues with plugin search in plugin browser
- Fix timeline minimap not drawing its contents
- Meson: fix use of sse flags on non-`x86_64` systems
- Fix modulators not being saved with the project
- Only recreate plugin port list when selected plugin changes (fixes lag in unrelated actions)
- Fix output track hash being saved as INT instead of UINT in channel leading to overflows and project corruption
- Fix stack smashing in recording manager when recording automation (pre-create dynamic array)
- Fix error when opening a project from a running instance
- Fix incorrectly freeing memory owned by GLib (`g_settings_schema_source_get_default()`)
- Fix various memory issues and possible NULL dereferences reported by GCC
- Fix Carla not being notified of buffer size changes
- Fix missing icons in mixer and transport display
- Fix automation not being drawn when the line is vertical

### Removed
- Remove unused widgets/files
- Remove unnecessary widget properties from UI files

## [1.0.0-alpha.26.0.13] - 2021-10-24
### Fixed
- Fix occasional error when dragging objects to negative positions
- User manual: fix build issues with some translations
- Fix error when activating nudge action on no selections
- Fix autosave interval of 0 not being respected
- Fix piano roll keys not making sound on the correct track
- Fix UI not refreshing after resetting fader value
- Fix error incorrectly being thrown during sample processing

## [1.0.0-alpha.26.0.1] - 2021-10-22
### Added
- Add gain to audio regions
- Allow changing BPM via text input
- Add BPM detection option for audio regions
- Allow changing fade in/out in audio editor
- Add more error handling in various places

### Changed
- Update Portuguese (Brazil), Japanese, Swedish, Indonesian translations
- Throw proper error when `dlopen()` failed on lv2 plugin
- Don't throw non-fatal error when timeline selections cannot be pasted
- Show error message when failed to serialize project when closing main window
- Don't allow auto-disarming tracks while recording
- Make steps to reproduce and other fields mandatory in bug report dialog

### Fixed
- Fix audio fades not being applied properly during processing
- Fix incorrectly allowing connecting sends from Master
- Fix graph export not working
- Fix sends not being added properly in graph export
- Fix error when lowering BPM when an audio region exists
- Fix error when destroying main window
- Fix error not being set when LV2 plugin fails to instantiate
- Fix crash when too many UI events are received
- Fix fade in/out and loop start/end not being stretched properly when BPM changes
- Add missing curve algorithms to GSettings schema

## [1.0.0-alpha.25.1.22] - 2021-09-11
### Added
- Show message when attempting to delete undeletable tracks

### Changed
- Always use carla discovery binary installed with zrythm
- Only change BPM/time signature when starting DSP processing (queue BPM/time signature changes)
- Update zix utils
- Use fallback image if failed to get screenshot for bug report
- Update Chinese (Simplified), Ukrainian, Portuguese, Spanish, Japanese, Russian translations

### Fixed
- Fix error when moving BPM automation point to position 0
- Fix error when changing BPM/time signature with scroll wheel
- Fix error during playback after changing a MIDI track name
- Fix positions not being updated correctly when changing beat unit

## [1.0.0-alpha.25.1.1] - 2021-09-06
### Added
- Best fit zoom on timeline
- Add more info to `--version`

### Changed
- Build vamp plugins ported from QM vamp plugins
- DSP optimization: cache automation track ports and clip editor region/track
- Optimization: use hashtable for looking up tracks by name hash
- Process UI events before performing actions
- Don't throw error if clip editor has no track when drawing piano roll keys
- Don't throw error if no track is hit when DnDing into the tracklist

### Fixed
- Fix segfault when double clicking on port in plugin inspector
- Fix error when renaming track that has sends
- Fix MIDI note indices not being updated properly when undoing deletion
- Fix zoom controls/shortcuts not working in editor
- Fix error when moving MIDI region to another track
- Fix GtkSourceView language spec path being hardcoded to version 4
- Fix plugins not being instantiated before connecting when duplicating tracks

## [1.0.0-alpha.25.0.1] - 2021-09-04
### Added
- Show indicator if region is looped
- Handle audio editor in editor event viewer
- Make event viewer columns reorderable
- Allow clamping to nearest acceptable position when moving region markers
- Show velocity values during UI actions
- Draw horizontal lines in velocity editor

### Changed
- Update Arabic, Thai, Chinese (Simplified), Japanese, Russian translations
- Only show 1 decimal point for positions in event viewers
- Split regions normally (destructively) if not looped
- Only allow merging unlooped regions
- Make vertical range selection space in timeline smaller
- Do not throw error if waveform widget does not have R channel data to draw
- Change env variable from `NO_SCAN_PLUGINS` to `ZRYTHM_SKIP_PLUGIN_SCAN`
- Do not reallocate memory for all ports when changing block length
- Make drum mode a per-track setting instead of per-project

### Fixed
- Fix sort by position/pitch/velocity in event viewers using alphabetical sort instead of int/position sort
- Fix UI not being refreshed when ramping velocities
- Fix error when merging regions
- Fix error when saving after undoing a region split
- Fix error when loading project after redoing a region split
- Fix audio files in pool sometimes being overwritten by other files
- Fix crash when renaming a track that has children routed to it
- Fix invalid memory usage in event viewer
- Fix error when loop-resizing audio regions from the left side
- Fix occasional meter-related segfault on startup
- Fix crash when moving playhead with snap keep offset enabled
- Fix snap keep offset not snapping to nearest snap point
- Fix attempting to open DSSI and LADSPA plugins without carla

### Removed
- Remove invalid check when duplicating audio regions
- Remove some unused files and dead code

## [1.0.0-alpha.24.0.1] - 2021-08-27
### Added
- Add SIGTERM handler that gracefully shuts down the application
- Link arranger selections in event viewers
- Highlight pressed notes in the piano roll from any source

### Changed
- Port suil improvements from upstream

### Fixed
- Fix error when loading some LV2 plugin UIs
- Fix carla plugins being unnecessarily instantiated during clone
- Fix occasional error when removing multiple regions
- Fix position label in event viewer not displaying negative positions properly
- Fix port identifiers not being serialized correctly
- Fix UI not being refreshed when changing velocity values
- Fix automatically armed tracks not being automatically disarmed after loading a project
- Fix MIDI channel send ports not being cleared on each run
- Fix channel MIDI output incorrectly being marked as a track port on project load
- Fix meters stopping drawing after autosave
- Fix hardware devices not being connected to existing tracks after loading a project

## [1.0.0-alpha.23.0.1] - 2021-08-26
### Added
- Submit compressed log file along with anonymous error reports
- Ability to dither on export
- New logarithmic curve algorithm
- User manual: add XRUN definition to glossary

### Changed
- Use hashtable to speed up dsp graph calculation
- Refactor & optimization: store owners on each object
- Change MIDI track and open UI button icons
- Refactor: add GError-based error handling for all undoable actions
- Update French, Japanese, Portuguese, Russian, Chinese (Simplified), Norwegian, Turkish, Ukrainian translations
- Use escaped name when drawing regions and markers
- Don't recalculate the graph every time the user clicks on a region
- Various DSP optimizations
- Skip autosave if in the middle of an arranger action
- Add more error handling when instantiating plugins and applying states
- Clone project before saving and save clone (ability to save yaml in a separate thread in the future)
- Don't allocate buffers for ports not used in the DSP graph (memory usage optimization)
- Refactor port connections into global port connections manager
- Refactor: Use track name hash to identify tracks instead of positions
- Don't recalculate graph when moving tracks
- Disable ability to record on chord track until implemented
- Don't attempt to show bug report dialog if main window doesn't exist
- Use `carla_save/load_plugin_state()` to save/load carla states
- Silence all output ports exposed to JACK when idle-processing
- Copy state directories instead of instantiating plugins when cloning (speeds up project saving)

### Fixed
- Fix changing marker name not taking effect on the UI
- Fix error when attempting to auto-scroll in a hidden arranger
- Fix error when attempting to loop-resize objects in timeline from the right side when resulting end position would be negative
- Fix tracks not being copied/moved inside foldable tracks correctly in some circumstances
- Fix tracks getting deselected when CTRL+dragging in tracklist
- Fix rare segfault in LV2 UI code (suil)
- Fix error when selecting a file in a generic LV2 UI

### Removed
- Remove some non-realtime calls from realtime functions reported by stoat

## [1.0.0-alpha.22.1.11] - 2021-08-07
### Added
- Add Thai language

### Changed
- Update Chinese (Simplified), Japanese, Portuguese translations

### Fixed
- Fix segfault when requested LV2 plugin UI not found
- Fix error when saving project with uninstantiated plugins
- Fix crash when moving a track
- Fix error when attempting to resize MIDI notes

## [1.0.0-alpha.22.1.1] - 2021-08-07
### Added
- Set window title and role when detaching tabs
- Add automatic bug reporting option
- Add json-glib dependency
- User manual: add Zchan images
- Add opt-in popup for checking for updates
- Move to start of double-clicked region in the editor
- Add line wraps to track comments

### Changed
- Allow specifying primary and secondary user shortcuts
- Always build with libcurl
- Change `phone_home` option to `check_updates`
- Redesign bug report dialog (add text input fields and buttons to send via email/sourcehut/automatically)
- Switch to resize-loop when attempting to resize-only a selection that contains a mix of looped and unlooped objects
- Propagate errors using GError for tracklist selections actions
- Show "(!)" on channel slot if plugin instantiation failed

### Fixed
- Fix app.goto-prev-marker user shortcut not being read
- Fix shift-m not working for muting objects
- Fix copy-paste and cut-paste not working in editor
- Fix editor size becoming larger when double-clicking region
- Fix playhead jumping to the start of a region in the editor if placed at the end of the region
- Fix quantize/quick-quantize not working in the editor
- Fix carla plugins becoming disabled when loading a preset
- Fix crash when selecting "overwrite events" recording mode
- Fix segfault in port code
- Fix click and drag to move playhead not working in editor ruler
- Fix incorrectly assuming the current version is not the latest version when checking for updates fails
- Fix crash when pressing right arrow on MIDI notes ending before the region start
- Fix error when moving folder track
- Fix folded track objects being visible in the timeline
- Fix error when changing preroll from 2 bars to none
- Fix timeline drop highlighting not taking into account folded tracks
- Fix crash when right clicking on an audio region and applying a function
- Fix stretching of MIDI regions not working

## [1.0.0-alpha.22.0.1] - 2021-08-01
### Added
- Add Turkish translation
- Install `zrythm_lv2apply`
- Allow renaming track lanes
- Allow applying audio functions on audio regions from the timeline
- Add nudge actions
- Add option to edit audio in an external app
- Add linear fade in/out and nudge functions in audio editor
- New dependency: vamp-plugin-sdk

### Changed
- Allow bouncing audio regions
- DSP refactor: use common time struct for processing
- Update Chinese (Simplified), French translations
- User manual: add section for mascot in user media chapter
- Don't allow resizing objects if not all selected objects are resizable

### Fixed
- Fix non-fatal error when attempting to split region at its start
- Fix recording incorrectly being handled for auditioner tracks
- Fix track lane not being correctly fetched when drawing tracks
- Fix crash when moving a region to another lane and undoing
- Fix audio regions not being redrawn when a function is applied
- Fix crash when showing changelog on MacOS
- Fix move action not being created when resizing an dmoving notes simultaneously

### Removed
- Remove unused source files for track lanes

## [1.0.0-alpha.21.0.13] - 2021-07-22
### Added
- Add accelerators to toolbox tooltips

### Changed
- Disable saving in save dialog shown when closing a project on trial version
- Silence debug messages printed when drawing audio regions

### Fixed
- Silence non-fatal error when detaching panels on some systems
- Fix error when dragging folder track into itself
- Fix dragging markers track inside folder track causing a crash
- Fix crash when deleting a track inside a folder track
- Fix crash on MacOS when loading a project
- Fix error when closing project containing folder tracks
- Fix error when loading project where an audio region is selected

## [1.0.0-alpha.21.0.1] - 2021-07-21
### Added
- Add monitor toggle to audio tracks
- Add Vietnamese translation
- Add metronome count-in
- Add recording preroll option
- Ask for save on close
- Add sends/MIDI FX to mixer channels
- Make all notebook tabs detachable
- Allow importing multiple files at the same time
- Allow overriding keyboard shortcuts
- User manual: add MIDI bindings section

### Changed
- Update Chinese (Simplified), Japanese, Russian translations
- Link reveal status of channel sends/inserts/MIDI FX on mixer
- Prettify file info in file browser
- User manual: move Shortcuts section

### Fixed
- Fix broken news button URL
- Fix crash when selected auditioning instrument is not found on startup
- Fix passing incorrect widget in file auditioning controls widget signals
- Fix error when attempting to audition MIDI file
- Fix incorrectly allowing the user to undo mid-sequence

### Removed
- Remove trial time limit

## [1.0.0-alpha.20.0.1] - 2021-07-15
### Added
- Add meson option for native build
- Add dots to position/BPM displays
- Add Indonesian translation
- Add graph SVG export
- Record incoming MIDI CC events into automation lanes
- Add ability to change instrument
- Add ability to save/load presets from plugin inspector
- Show port groups in plugin inspector
- Add ability to perform multiple undoable actions in sequence
- Add ability to change direct out for multiple tracks (and to create a new group to route to)
- Add folder tracks

### Changed
- Enable link time optimization (LTO) by default
- Draw even less detail on audio regions when CPU usage is above 40%
- Update French, Chinese (Simplified), Japanese, Ukrainian translations
- Make group tracks foldable
- Silence unnecessary MIDI event logs
- Do not attempt to free swh lv2 plugins (upstream issue)

### Fixed
- Fix channel sends being outside pre-fader group in graph exports
- Fix various issues on MacOS
- Fix missing libm dependency on lv2apply used during tests
- Fix error when loading project with duplicated audio region
- Fix multiple tracks losing their order when moved
- Fix incorrectly allowing 0 tracks to be selected when ctrl-clicking on track
- Fix default loop range being 5 bars long instead of 4 bars
- Fix memory leak in track processors
- Fix crash when setting listen status on MIDI track

## [1.0.0-alpha.19.0.1] - 2021-06-24
### Added
- Add bookmarks and filters to file browser
- Add MIDI/audio auditioning support to file browser
- Make MIDI note/velocity colors take velocity into account
- Allow MIDI learn on track sends

### Changed
- Show error message when plugin UI fails to open
- Always copy/reflink audio pool files to backups instead of creating symlinks
- Update user manual sections: getting started, projects, configuration, plugins, audio and MIDI files
- Lower GLib requirement to 2.64
- Update Spanish, Russian translations
- Update popup file browser and refactor common logic with panel file browser
- Draw MIDI note velocities as lollipops
- Center velocities under MIDI notes

### Fixed
- Fix crash when adding a MIDI FX track
- Fix rare error when attempting to queue metronome samples

## [1.0.0-alpha.18.2.1] - 2021-06-09
### Added
- Add AppImage support

### Changed
- Update Chinese (Simplified), Russian, Ukrainian, Polish translations
- Install org.zrythm.Zrythm.desktop instead of zrythm.desktop
- Various DSP/UI optimizations

### Fixed
- Fix crash when attempting to get last n lines from log file before it is initialized
- Fix undo stack indices not getting updated when removing actions
- Fix error when opening automation lanes on master

## [1.0.0-alpha.18.1.1] - 2021-06-06
### Added
- Make track mute/solo/listen/mono/record MIDI bindable

### Changed
- Only listen to MIDI notes within the first beat during moving
- Don't show non-fatal error when RtAudio backend fails to initialize
- Disable hardware processor callback when disabling the audio engine (fixes occasional error when closing projects)
- Call cleanup() on all LV2 plugins except helm

### Fixed
- Fix error when removing unused clips from the pool

### Removed
- Remove ability to change JACK buffer size on the fly on Windows

## [1.0.0-alpha.17.1.22] - 2021-06-04
### Changed
- Update Greek, Spanish, Chinese (Simplified), Norwegian Bokmal, Japanese translations
- Annotate releases with changelog
- Make LV2 plugin <=> UI communication buffers larger

### Fixed
- Fix crash when undoing deletion of multiple inserts
- Fix audio track inputs not being available after loading a project
- Fix bounced regions not starting at start marker when bouncing tracks
- Fix error when attempting to open plugins with carla when zrythm is built without carla support

## [1.0.0-alpha.17.1.3] - 2021-06-02
### Added
- Highlight bass note in piano roll
- New xxhash dependency
- Allow enabling/disabling tracks
- Add bounce option to disable track after bounce
- Add context menu to channels in mixer
- Implement cut/copy/paste/delete/select all/deselect all for plugin slots
- Allow deleting plugins with delete key
- Automatically adjust timeline length
- Add CSS theming for list boxes
- Auto-generate and install CLI completions for fish and bash
- Remove unused or untracked pool files during save

### Changed
- Don't pause engine when performing mute/solo/listen actions
- Change version requirement of libcyaml to 1.2.0
- Convert plugin presets combo box to list box
- Don't populate presets list box for uninstantiated plugins
- Save pool ID or base64 MIDI instead of absolute paths in track creation actions
- Use FLAC to save imported clips if bit depth < 32
- Temporarily disable audio engine and show modal dialog during save (fixes random errors during save)
- Save the project struct directly instead of memcpying it
- Skip re-writing existing files to audio pool
- Create symlinks to main project's pool files when saving backups
- Defer autosave if sound is playing on master

### Fixed
- Fix notes from multiple regions being selected when ramping velocities
- Fix audio recording including audio from other regions
- Fix bars (position) not being passed to LV2 plugins
- Fix incorrect expected position for next cycle when processing LV2 plugins
- Fix some broken links (CGit -> Gitea migration)
- Fix error when opening scripting window
- Fix GUI freezing when clicking inside piano roll
- Fix hangs when no more objects are available in object pools
- Fix UI events being spammed when changing a control value
- Fix select/deselect all not working in editor
- Fix crash when dragging non-existing plugin to slot on MacOS
- Fix naming scheme of duplicate files in the audio pool
- Fix audio pool not being saved to backup projects
- Fix too many `ET_PLUGIN_STATE_CHANGED` events being sent
- Fix error when loading project containing audio files with different sample rate

## [1.0.0-alpha.16.1.1] - 2021-05-14
### Added
- Allow choosing CSS theme from preferences
- Allow choosing icon theme from preferences
- Add options to follow playhead and auto-scroll on edges

### Changed
- Update Portuguese, Chinese (Simplified), Ukrainian translations
- Install `zrythm_launch` on Mac
- Change insert/remove range button icons

### Fixed
- Fix sends not copied when duplicating an instrument track
- Fix errors when removing project range
- Fix arrangers losing focus when pressing a key
- Fix crash when changing marker track color
- Fix dynamic library being used when using glib subproject

## [1.0.0-alpha.16.0.37] - 2021-05-07
### Added
- Add preliminary support for LV2 options interface
- Import nanovg & make using OpenGL easier
- Show address and binary file in backtraces if available

### Changed
- Require glib 2.68 or above
- Update glib meson wrap to 2.68.1
- Update French, Chinese (Simplified) translations
- UI theme: make bright green more blue

### Fixed
- Fix possible crash in slider widgets
- Fix metronome not being played on bar 1
- Fix wrong buffer offsets being passed when splitting dsp cycles
- Fix crash when double clicking on empty slot
- Fix segfault when `port_get_dest_index()` fails
- Fix gtksourceview language specs not found on MacOS installer build
- Fix plugin bypass state not being restored on project load

## [1.0.0-alpha.16.0.12] - 2021-05-02
### Added
- Print required/optional lv2 options during instantiation
- Abort instantiation of LV2 plugins with required options that are not supported
- Pass nominalBlockLength to LV2 plugins

### Changed
- Generate copyright name and years from meson config
- Pass correct window title to external LV2 UIs
- Pass data access/instance access to plugins as features
- Mark state:makePath as supported feature
- Set minBlockLength to 0 and maxBlockLength to 4098 for LV2 plugins
- Create new plugin settings if failed loading current ones instead of crashing

### Fixed
- Fix passing invalid ui:parent to lv2 plugins (suil automatically adds it)
- Fix LV2 event buffer not being reset before run() for output ports
- Fix crash when instantiating plugins with > 200 parameters

### Removed
- Remove support for powerOf2BlockLength and fixedBlockLength LV2 features

## [1.0.0-alpha.16.0.1] - 2021-04-30
### Added
- Make fader and piano roll highlight colors themeable
- Add bar and beat snap options
- Allow bouncing tracks pre-inserts, pre-fader, post-fader, or with parents
- Add plugin author filter in plugin browser
- Add meson option for carla 32bit binaries on windows
- Allow soloing/muting multiple tracks
- Allow changing color for multipler tracks
- Add title field to export dialog
- Allow changing monitor output device ports (JACK only)
- Implement channel listen
- Add option to unsolo/unmute/unlisten all tracks
- Add mute/listen/dim knobs to monitor section
- Add mono/dim/mute functionalities to monitor section

### Changed
- Update French, Japanese, Greek, Chinese (Simplified), Ukrainian translations
- Show value readings on some knobs
- Don't serialize port internal value type
- Pass hardRTCapable and threadSafeRestore LV2 features to plugins during instantiation
- Stop and restart engine when LV2 plugin does not support thread-safe state restore
- Do not re-save the state when instantiating plugins unless state dir does not exist

### Fixed
- Fix fetching latest version in installer build
- Fix error when duplicating tracks with sends/direct outs
- Fix soloed tracks being silent when routed to groups
- Fix LV2 plugin states not being saved correctly when there are files involved
- Fix error when opening generic UIs for bridged plugins
- Fix error when deleting a track when mixer selections exist after the track
- Fix (pw) being shown for MIDI in bot bar when using JACK audio backend with a non-JACK MIDI backend
- Fix error when moving plugin from inserts to MIDI FX
- Fix playhead not being redrawn when moving back to cue point
- Fix incorrect backend being selected in first run wizard

### Removed
- Remove makePath feature for state saving for LV2 plugins (handled by lilv)

## [1.0.0-alpha.15.0.1] - 2021-04-03
### Added
- Implement MIDI fader mute
- Implement MIDI fader velocity multiplier
- Add versions to structs
- Add all automatable CC controls to MIDI tracks
- Add output gain control on audio tracks
- Make track sends automatable
- Make beats per bar and beat unit automatable
- Add Contributor Certificate of Origin
- Allow vertical moving when creating MIDI notes

### Changed
- Update Japanese, Chinese (Simplified), Ukrainian, Spanish, French translations
- User manual: prettify scheme API docs (use parens)
- User manual: use Furo theme for HTML docs
- User manual: use existing API docs if can't generate updated ones
- Set last known automation value on controls when moving playhead manually
- Silence some excessive log output
- Disable musical mode
- Only add control ports to processing graph if they have sources or automation
- Don't allow more than max MIDI events in the same processing cycle when processing carla plugins
- Make graph calculation faster (use port's automation track cache instead of searching)
- Skip SIGTRAP in `zrythm_gdb`
- Show error popup if failed to lock down unlimited memory
- Don't allow setting audio region loop end beyond clip frames
- JACK: show error message when failing to connect to system ports
- Hide hidden tracks from mixer view as well
- Fail if plugin settings cannot be initialized
- Remove link after duplicating a linked region
- Use semaphore to avoid saving project while performing actions
- Always build some DSP code with full optimizations
- Use plugin hints to check if carla plugin has custom UI

### Fixed
- Fix latest release dialog
- Fix pipewire deadlock during startup
- Fix non-stop errors when selecting JACK MIDI backend with dummy audio backend
- Fix error when unpinning chord track
- Fix crash when using ramp tool on arranger objects in timeline
- Fix project failing to serialize when plugin bank has no presets
- Fix crash on buffer overflow when sending events to LV2 UIs
- Fix LV2 presets not being read properly
- Fix crash when track's output cannot be found
- Fix creating new projects from templates not copying over plugin states
- Fix crash when closing project with automation
- Fix crash when BPM automation exists
- Fix pool not being saved when saving projects in a different location
- Fix error when deleting automation region
- Fix port flags not being copied correctly
- Fix crash when instantiating ZLFO UI
- Fix error when deleting markers, scale objects or chord objects
- Fix bounced track material not moved to start marker
- Fix bounced audio skipping first MIDI note
- Fix lag when right-clicking on plugins in the plugin browser
- Fix endless note being played at end of MIDI region
- Fix Zrythm enabled/gain controls not working in LV2 generic UIs

### Removed
- Remove GOVERNANCE document

## [1.0.0-alpha.14.1.2] - 2021-03-14
### Added
- Add context option to select UI if plugin has multiple

### Changed
- Wrap version in triple backticks in bug report template
- Store selected LV2 UI in plugin settings
- Make bug report dialog top level if main window is not initialized
- Use timestamp for log file in /tmp

### Fixed
- Fix blocking when attempting to connect ports
- Fix errors when deleting automation points or chords
- Fix plugin collections not being recognized
- Fix crash on right-click -> paste
- Fix chord track not receiving MIDI panic messages
- Fix chord higlighting not working properly
- Fix LV2 external UIs not opening/closing properly
- Fix attempting to get a backtrace crashing on some systems

### Removed
- Remove global option to force generic UIs
- Remove global option to bridge unsupported UIs

## [1.0.0-alpha.14.0.1] - 2021-03-12
### Added
- Make LV2 parameters automatable if numeric
- Add ctrl-shift-scroll to piano roll
- Add ctrl-shift-scroll to timeline/tracklist
- Generic UI support for carla-based plugins
- Add emblems to plugin browser icons to show that something is selected
- Add emblem to log viewer button to indicate unseen warnings
- Add action stack info to bug report template
- Add pcre2 dependency for internal regex operations

### Changed
- Only read log domains from the environment once
- Internal refactor of plugin ports: merge lv2-only port logic with generic ports
- Handle control type properly for carla-based plugins (logarithmic, integer, etc.)
- Install carla binaries in lib/zrythm/carla instead of bin
- Wait for all DSP threads to become idle during startup
- Update Chinese (Simplified), Norwegian Bokmal, Japanese, French translations
- Split plugin preferences from plugin descriptors and save plugin preferences in plugin-settings.yaml
- Only connect event ports that support MIDI
- Use more descriptive title in plugin window
- Add version dependency on RtMidi
- Make position struct slimmer: remove bars/beats/sixteenths/ticks/subticks and only remember ticks and frames
- Use libbacktrace to get backtraces with line numbers
- Read meson project version from VERSION file
- Only start drag actions when a movement threshold is reached
- Enable resize cursors within 8 pixels of object edge instead of 9
- Force cyaml subproject until upstream fixes issue with restoring floats

### Fixed
- Fix LV2 plugins not being freed
- Avoid nans and non-finite numbers in logarithmic port value conversions
- Fix some race conditions
- Don't attempt to update window title when closing the UI for carla plugins
- Fix position info not being sent to plugins
- Fix `--yaml-to-zpj` option doing the opposite
- Fix crash when pressing delete with nothing selected
- Fix crash when changing the direct out of a track and only selecting a category
- Do not allow deletion of undeletable tracks (master/tempo/chords/etc.)
- Fix opened directories not being closed when scanning for VST2/VST3/SFZ/SF2
- Fix crash when attempting to open a project created with RtMidi after removing the device
- Fix chord track sometimes skipping notes
- Fix non-fatal error when attempting to export after loading a project with a plugin that failed to instantiate
- Fix chord editor not redrawing playhead during playback
- Fix crash during autosave after loading a project containing a carla plugin that failed to instantiate

## [1.0.0-alpha.13.1.1] - 2021-03-05
### Added
- Allow changing engine buffer size while running
- Add event loop to engine to process change requests in GUI thread
- Show changelog dialog when running official release for first time
- Show notification when new version is out on official builds
- User manual: add info about memory locking and open file limits in system requirements
- User manual: implement lexer for `tree` command output in project section
- User manual: document `ZRYTHM_DSP_THREADS` in environment vars page

### Changed
- Enable static analysis when strict flags enabled
- Add HOT attribute to functions called very often
- Add NONNULL attribute to functions to enforce checks at compile time
- Lower number of log lines in error dialog to 100
- Localize pre-startup output (eg, `--help` output)
- User manual: convert toc dumps to overview pages in configuration/getting started/projects/zrythm interface chapters
- User manual: rewrite project structure page
- Use aligned memory allocation in LV2 event buffer

### Fixed
- Temporarily disable JACK transport during export (fixes freeze when running as a JACK client)
- Fix stack overflow in meter logic and live waveform widget
- Fix error when changing JACK transport BPM from another app while being a client
- Fix use of `[[` in `zrythm_launch` script
- Fix memory corruption when using latest LV2
- Fix warnings when building with `-Doptimization=3`

## [1.0.0-alpha.13.0.4] - 2021-02-28
### Added
- Allow MIDI learn on transport controls
- Add dir for user scripts
- Add `--cyaml-log-level` option

### Changed
- Silence many unnecessary logs
- Update Chinese (Simplified), French translations
- Optimize some DSP functions
- Optimize RAM usage for ports (only allocate what is needed)
- Use new libaudec v0.3
- Read example scripts from disk and test them
- Switch to `g_application_add_main_option_entries()` for parsing CLI args
- Use common procedure for pausing/restarting engine
- Handle plugin clone failures more gracefully
- Only attempt to disconnect ports if already connected

### Fixed
- Re-fix AU plugin scan on MacOS
- Fix number of port connections not being updated in inspector when connecting ports
- Fix MIDI note being skipped after transport loop
- Fix errors when loading projects after disconnecting a MIDI device
- Fix errors on wayland when instantiating LV2 plugins
- Fix error when changing piano roll MIDI modifier
- Fix lag when muting/soloing tracks
- Fix unnecessary logs being printed when using CLI commands
- Fix writing to invalid memory when connecting ports
- Fix error when selecting nothing with erase tool in automation editor
- Fix crash when opening project with non-found carla plugin
- Fix crash when closing a project containing plugins that failed to initialize
- Fix locales mismatch between GSettings schema and Zrythm UI
- Fix data not being moved properly when disconnecting ports

## [1.0.0-alpha.12.0.1] - 2021-02-20
### Added
- Add LV2 extension to pass host info to plugin
- User manual: add JACK transport sync section
- User manual: add info about MIDI/audio recording
- User manual: add more details about port connections

### Changed
- Change track pin/unpin logic

### Fixed
- Fix crash when audio region's start position is within the current cycle
- Fix right click in timeline not selecting the object
- Fix SF2s not being scanned
- Fix error when duplicating regions to other lanes/tracks
- Fix crash when undoing deletion of an instrument track after routing a MIDI track to it
- Fix regions on MIDI tracks routed to instrument track marked for export not being played during export
- Fix post-install script not running when it should
- Fix JACK transport behavior
- Fix plugin descriptor cache being ignored sometimes
- Fix regions not being inserted at their original index when undoing deletion
- Fix error when undoing region splitting
- Fix example script

## [1.0.0-alpha.11.1.1] - 2021-02-15
### Added
- Allow sending to plugin sidechain inputs (pg:sideChainOf)
- Add horizontal scrollbar to modulators tab
- Install `zrythm_lldb` on MacOS

### Changed
- Deactivate engine before closing main window
- Update Portuguese (Brazil), Russian, Chinese (Simplified), Persian translations
- Only redraw meters if values changed
- `--gdb` and `--valgrind` options moved to shell scripts `zrythm_gdb` and `zrythm_valgrind`
- Use GOptionEntry to parse command line args
- Remove empty tracks when importing a MIDI file
- Update lv2/lilv/sratom/serd/sord wraps
- Convert meson post-install script to shell script

### Fixed
- Fix error when moving plugin to duplicated track after connecting ports
- Fix error when pressing delete in the tracklist
- Fix making a selection with Ctrl held down not appending to the selection
- Fix recorded audio not being saved properly
- Fix merge button state not updated when selecting objects with the select tool
- Fix invalid serialization of track edit actions
- Fix MIDI panic not sent when performing actions

## [1.0.0-alpha.11.0.2] - 2021-02-12
### Added
- Print available GTK modules on startup
- Native PulseAudio backend
- New icons
- Pass UI scale factor to plugin UIs

### Changed
- Open LV2 plugin UIs with carla by default on Windows
- Don't jump to cue point when temporarily pausing to perform actions
- Send MIDI panic when pausing to perform actions
- Move port connections and MIDI CC bindings from left panel to main notebook (new)
- Allow opening LV2 plugins with CV ports with carla
- Update Portuguese (Brazil), Greek, Chinese (Simplified) translations
- Allow dropping MIDI files into existing tracks
- Optimize track edit actions: do not clone tracks for simple actions
- Propagate changes from/to LV2 ports designated as lv2:enabled to/from the zrythm-provided enabled port
- Delegate process bypassing to plugin if lv2:enabled-designated port is present
- Only send LV2 control val change event to UI if the value changed

### Fixed
- Fix crash after a while when using pencil tool
- Fix double clicking on non-selected track opening up color chooser
- Fix crash when filling automation with pencil tool
- Fix MIDI import not adding note offs when velocity is 0
- Fix crash when importing MIDI file that uses note ons with velocity 0 as note offs
- Fix AU plugin scan on MacOS
- Fix incorrectly freeing plugin manager's lilv nodes when freeing an lv2 plugin
- Fix icons drawn with cairo not taking scale factor into account
- Fix ruler font not taking scale factor into account
- Fix curviness being lost when duplicating automation regions
- Fix crash when moving plugin from mixer slot to empty space in tracklist or mixer
- Fix modulator macros not being properly connected to graph

## [1.0.0-alpha.10.0.1] - 2021-02-02
### Added
- Font scale option in preferences
- Ability to rename macro buttons
- Ability to edit track comments (undoable)
- Ability to cancel export/bounce
- Add reset/bind CC/view info context options to modulator controls and macro knobs
- OPUS export support

### Changed
- Update Catalan, Japanese, Chinese (Simplified), Ukrainian translations
- Remember port connections when undoing plugin removal
- Refactor filling of audio/MIDI events from regions during playback
- Pause transport before performing actions
- Changing time signature is now undoable
- User manual: clarify control behavior with respect to automation events

### Fixed
- Fix error when region contents start before global 1.1.1.0
- Fix error when attempting to select automatable in modulator track
- Fix error when instantiating LV2 plugin that has multiple params with same name through carla
- Fix marker having negative y coordinate when font scaling is high
- Fix metronome calculation for beats
- Fix crash when routing instrument track to audio group
- Fix crash when removing modulator that has a routed output
- Fix stuck note when note ends after region end
- Fix bug report dialog not showing text when text contains URIs
- Fix error when selecting pencil tool on Windows
- Fix wrong binaries path passed to carla on installer versions
- Fix crash when loading plugins that fail to instantiate
- Fix silent note after transport has looped
- Fix plugins receiving events at invalid times when cycle splitting occurs
- Fix non-fatal error when not selecting a port in the port selector window and pressing OK
- Fix 1/4 time signature skipping every 3 beats

### Removed
- Remove beat unit enum from transport

## [1.0.0-alpha.9.0.1] - 2021-01-24
### Added
- Show warning when attempting to open deprecated LV2 UIs
- Warn if lv2 plugin contains dynamic link to illegal libraries (such as qt)
- Modulator macro buttons
- Dropdowns to select from undo/redo history
- Home key shortcut to move playhead to start of session

### Changed
- Only attempt to get latency on LV2 plugins that have a reportsLatency port
- Update Portuguese, French, Chinese (Simplified), Portuguese (Brazil), Ukrainian translations
- Update all carla plugin params before saving state
- Automatable selector: start insert/modulator/midi fx counts from 1 instead of 0

### Fixed
- Fix objects not being resizable beyond bar 128
- Fix infinte loop when attempting to set direct out of track to a selected track
- Fix crash when adding a modulator
- Fix not being able to move playhead past 128 bars
- Fix wrong tracks being deleted when undoing track duplication
- Fix crash after transport loops back when recording automation in touch mode
- Fix error when adding automation lanes
- Fix crash when recording audio for a long time
- Fix current edit mode not selected on startup

## [1.0.0-alpha.8.0.1] - 2021-01-18
### Added
- Moving region markers is now undoable

### Changed
- Do not attempt to draw regions if visible width is 0
- Refactor each plugin action into a single mixer selections action
- Refactor each track action into a single tracklist selections action
- Add Arch Linux to exceptions in trademark policy

### Fixed
- Fix automation region mute
- Fix crash when cloning from insert into new track
- Fix crash when moving two effects in the mixer and the new position overlaps one of them
- Fix start/end markers being deletable
- Fix right channel having lower volume after recording audio in mono
- Fix crash when reaching transport loop end and metronome enabled
- Fix crash when moving a plugin to a slot that has another plugin
- Fix crash when deleting all visible tracks
- Fix regions of hidden tracks being shown in the timeline
- Fix crash when holding down ctrl-d for a long time

## [1.0.0-alpha.7.1.1] - 2021-01-03
### Added
- Add `--gen-project` option to generate projects from Guile scripts
- Expose more Guile API
- Filter plugin list during search

### Changed
- User manual: Add more info about timeline editing
- User manual: Compact translations into 1 file for each language
- User manual: Use dark theme for html output
- Update Greek, Spanish, Portuguese, Chinese (Simplified), Ukranian, Russian, Italian, Portuguese (Brazil), Japanese, Galician, German translations
- Cache audio region fades

### Fixed
- Fix audio region fades not showing
- Fix not being able to change fade points in lane regions
- Fix plugin browser collection context menu being shown multiple times
- Fix crash when using `--gdb` and more than 100 env variables exist
- Fix compilation errors when building with latest carla-git
- Fix projects not being loaded properly when using JACK
- Fix protocol selection not working in plugin browser when collection or category is selected

## [1.0.0-alpha.7.0.1] - 2020-12-30
### Added
- Add reset right click option in categories in plugin browser
- Plugin collections
- Filter by protocol in plugin browser
- Snap to events and snap-and-keep-offset
- Recording modes: overwrite events, merge events, takes (mute previous)
- Projects are now samplerate agnostic
- Snap length option: last object

### Changed
- Do not show bug report popup within 8 seconds of previous one
- Move snap settings from main toolbar to each arranger
- Disarm transport record after recording
- Reference clip frames directly for audio regions instead of duplicating the buffers (lowers RAM usage)
- Improved region drawing performance (better caching)

### Fixed
- Fix file browser divider position not being loaded properly
- Fix preferences dialog crash when more than 60 audio input ports exist
- Fix clip not being duplicated when duplicating audio regions
- Fix selected language not being picked properly
- Fix lane regions not being shown when main region is not visible
- Fix MIDI notes being muted when region is looped and note starts before clip start marker
- Fix crash when quantizing in audio editor

## [1.0.0-alpha.6.0.1] - 2020-12-11
### Added
- Audio selection and functions (invert, reverse, normalize) in audio editor

### Changed
- Update Spanish, Norwegian Bokmal, Portuguese, Chinese Simplified, Portuguese (Brazil), Danish, Greek translations
- Move issue tracker from Redmine to Sourcehut
- Change wording in user manual: edit modes -> edit tools

### Fixed
- Fix audio clip being offset by region size when splitting an audio region and saving the project
- Fix crash when automating LV2 plugins that have multiple ports with the same label but different symbol
- Fix not being able to scroll past 21 bars after importing a MIDI file

## [1.0.0-alpha.5.0.1] - 2020-11-15
### Added
- Write audio files during recording (every 2 sec)
- F1 shortcut to open manual
- F2 shortcut to rename regions
- Add vertical flip action for automation
- Add menu option to reset plugin parameter values
- Add option to show automation values in automation editor
- Show border around selected UI section

### Changed
- Split graph cycle at loop points
- Update Russian, Portuguese, Japanese, German, Norwegian, Greek, French, Chinese, Italian, Spanish, Galician, Polish translations
- Remember tab selection in bot/left/right panels
- Update editor toolbar based on region type
- User manual: various updates
- Change deselect all shortcut from ctrl-backslash to ctrl-shift-a
- Show region name instead of track name in editor margin
- Update some icons (select, audition, sends, inserts, etc.)

### Fixed
- Fix chord pad not updating when a chord changes
- Fix select/deselect all actions not working
- Fix track not shown as selected in the tracklist when clicking on it in the mixer
- Fix critical error when moving track with automation
- Fix other regions not drawn in editor ruler when automation region selected
- Fix broken MIDI track inspector page on project load
- Fix chord pad remaining pressed during drag n drop
- Fix clip editor region's automation track index not being updated when switching automation lanes
- Fix/refactor copy-paste of arranger selections
- Fix crash when muting MIDI region
- Fix not being able to re-select a track's send target after selecting None
- Fix crash during recording when loop traverses back
- Fix crash when splitting chord region
- Fix SFZ/SF2 support
- Fix MIDI note getting skipped when traversing transport loop end
- Fix crash when deleting a region with erase mode

### Removed
- Remove bottom arranger from automation editor

## [1.0.0-alpha.4.0.1] - 2020-10-03
### Added
- Export stems

### Changed
- Update Greek, French, Swedish, Russian, Portuguese, Japanese, Spanish translations
- Warn instead of critical when port in LV2 preset is not found
- User manual: update export and getting started chapters
- Revamped export dialog
- Auto-generate localization header (so languages in code are in sync with LINGUAS)

### Fixed
- Fix non-fatal error when trying to edit scale in chord track
- Fix non-fatal error when right-clicking in piano roll using eraser
- Fix issues when opening a project using a different backend
- Fix track bouncing
- Fix build issues when `user_manual` meson option is enabled

## [1.0.0-alpha.3.0.1] - 2020-09-28
### Added
- Add hardware processor for controlling input hardware ports (can now record with RtAudio/RtMidi)
- Write to zrythm.log under the system's temporary dir until the actual log file is initialized

### Changed
- Update Japanese, Portuguese (BR), French, German, Chinese (Traditional), Swedish, Russian, Dutch, Spanish, Afrikaans, Greek translations
- Show whether this is the release version in `--version`
- Use atomic flag instead of port operation lock for bypassing the engine (fixes crash when deleting a MIDI track after bouncing it)

### Fixed
- Fix some widgets not showing in preferences dialog
- Fix runtime error when selecting nothing with eraser tool
- Fix various syntax errors in translations
- Fix missing `af_AZ` locale in GSettings schema
- Fix plugins not being instantiable the first time they are scanned

## [1.0.0-alpha.2.0.1] - 2020-09-25
### Added
- Add DSSI and LADSPA plugin support
- Support VST and LADSPA shells (libraries that contain multiple plugins)
- Add better error handling when opening plugin UIs
- Remember arranger zoom levels with projects
- Allow passing arguments to `zrythm_launch`
- Remember user choice when right-clicking on a plugin to instantiate it
- User manual: Add Glossary, Command Line and Environment pages
- Add `af_ZA` locale
- Add tests for PO files
- Add translation context to UI labels in Preferences dialog

### Changed
- User manual: Update various sections
- Updated Russian, Chinese (Traditional), Japanese, Spanish, Swedish translations
- Change `zh_Hans` to `zh_CN` and `zh_Hant` to `zh_TW`
- Port sphinx Makefile to meson
- Use latest Carla API

### Fixed
- Fix unnecessarily loading Carla plugin states for non-project plugins on project load
- Add missing nl locale in LINGUAS
- Fix attempting to open custom UIs on plugins that don't have any
- Fix crash when clicking on port selector in Tempo track automation lanes
- Fix track auto-arm

## [1.0.0-alpha.0.1.1] - 2020-09-18
### Added
- Increase open file limit on app startup
- Show popup when JACK shuts down
- Add test for deleting plugins
- Add more meson subprojects

### Changed
- Show underlying backends instead of RtAudio/RtMidi
- Place bounced material where the first region starts if bouncing regions
- Revise Trademark Policy
- Update Chinese (Traditional) translations

### Fixed
- Fix broken export/bounce (WAV and FLAC)
- Fix FreeBSD build
- Fix skipping setting of Carla engine options
- Fix crash when deleting a newly created MIDI track on an empty project
- Fix remembering position of divider between timeline and clip editor
- Fix crash when deleting a MIDI FX slot on a MIDI track
- Fix LV2 plugins not applying file selection changes

## [0.8.982] - 2020-09-15
### Added
- Allow renaming regions
- Add option to build with static libs where possible
- Add option to get Carla bridge/discovery binaries from custom dirs
- Add GOVERNANCE document
- Add framework for MIDI functions (such as legato)
- Allow changing track color and icon
- Add versioning info in PACKAGING.md
- Add toolbar to modulators with show UI/delete buttons
- Remember clip visibility status in the clip editor and scroll positions in each arranger when saving/loading projects

### Changed
- Rework snap options
- Update Spanish, Italian, French, Japanese, Portuguese, Chinese (traditional) translations
- Use new carla-host-plugin dependency instead of carla standalone, carla utils and carla native plugin
- Improve fftw dependency discovery
- Show icons in piano roll highlight combo box
- Show "(pw)" indicator if JACK is running through PipeWire
- Do nothing when duplicate action is invoked with nothing selected
- Use whereami to get absolute executable path (for passing to addr2line)
- Disable plugins that fail to instantiate when loading projects
- Skip MIDI events not 3 bytes long
- Add glib log domain (so that `G_MESSAGES_DEBUG=zrythm` works)
- Close RtMidi ports before attempting to open
- Only send data to JACK ports if they have connections
- Show warning when using JACK audio/MIDI backend with non-JACK audio/MIDI backend

### Fixed
- Fix crash when attempting to open automation tracklist on tracks without any automation tracks
- Fix markers icon missing on Windows
- Fix plugin creation action not remembering the number of plugins to create
- Fix crash when dragging chords into invalid locations in the timeline
- Fix all note ends becoming the same position when resizing MIDI notes
- Fix not being able to resize MIDI note start positions beyond region start position when resizing from left side
- Fix piano roll drum mode

## [0.8.911] - 2020-09-01
### Added
- Add right click + drag to delete objects with select tool
- Add MIDI CC bindings table
- Add port connections table
- Add modulator track
- Add ability to drop modulators in modulators tab
- Allow connecting modulators to controls

### Changed
- Make changing track name undoable
- Make changing fader value undoable
- Make balance control changes undoable
- Make control port value changes undoable
- Make MIDI CC binding actions undoable
- Update Italian, Portuguese, Chinese, Spanish translations
- Split Chinese translations to Simple and Traditional
- Minor fader redesign (use thinner handle with white line)
- Use `reproc_run_ex` API for running external commands
- Update minimum reproc dependency version to v14.1.0
- Change how knobs and bar sliders get their values (add snapped getter)
- Add scrollbars to port selector popover

### Fixed
- Fix scales not being editable
- Fix issue with editing markers
- Fix invalid knob drawing when zero point is negative
- Fix range action not being initialized on project load
- Fix enabling/disabling the port connection in port connections popover not being undoable
- Fix missing icon for Add button in port connections popover
- Fix drag n drop of chords from the chord pad into the timeline not working

## [0.8.868] - 2020-08-24
### Added
- Add recording mode selector
- Show line numbers in backtraces when possible
- Add recording test
- Add tooltips to track buttons
- Implement autofill for piano roll and timeline
- Implement free drawing in velocity editor
- Implement free drawing in automation editor
- Enable VST3 on GNU/Linux
- New optional dependency: lsp-dsp-lib
- Add chord pads
- Play chord on selected track when chord pad is pressed
- Allow drag and drop from chord pad to MIDI/instrument tracks
- Allow dragging MIDI and audio files directly into the timeline
- Add region merge action

### Changed
- Refactor recording code: use pause/resume for punch in/out and looping
- Update Portuguese, German, Japanese, Italian translations
- Use libaudec with minimp3 for mp3 import support
- Only create undoable action when all recording is finished (instead of per track/automation track)
- Ensure that recorded audio clip name is unique (fixes recorded audio being lost due to writing to the same file)
- Use SIMD-optimized DSP routines when available through lsp-dsp-lib
- Update user manual
- Ensure all drawn arranger objects are at least 1 pixel
- Process UI events immediately when performing actions
- Simplify code of conduct

### Fixed
- Fix segfault when opening project from the edit menu
- Fix backtrace memory leak on Windows
- Fix track solo not being drawn as hovered when hovered
- Fix Carla Rack (LV2) segfault on UI close
- Fix same descriptor instance being added to both cached VST descriptors and plugin manager
- Fix automation point coordinates sometimes being negative when drawing in the automation arranger
- Arranger: fix `region_at_start` not being set to NULL after freeing
- Fix crash when moving tracks when clip editor region was in the affected tracks
- Recording: fix track pause events being sent when nothing is being recorded
- Fix crash when undoing deletion of tracks with LV2 plugins with worker interfaces

### Removed
- Remove ffmpeg dependency
- Remove libgtop related code

## [0.8.797] - 2020-08-11
### Added
- Add plugin gain port
- Add scroll wheel support for fader
- Add mono compat switch on group tracks and master
- Add insert silence/remove range actions
- Add mono toggle and gain knob on audio track inputs
- Add test for audio track deletion
- Add new widget for buttons with menus
- Add volume slider for metronome
- Add punch mode switch to record button for punch in/out recording
- Ruler: add option to switch to real time display

### Changed
- Refactor fader buttons into separate widget
- Show host system type in version info
- Check number of ports are the same when cloning plugins
- Hard limit master bus and monitor out to +6db
- Allocate system command output instead of passing buffer
- Integrate new icons
- Revise trademark policy to allow modifications by FSF-approved distros or for fixing acknowledged bugs and CVE vulnerabilities
- Update bot panel tab tooltips
- Refactor recording code: split cycle at punch and loop points
- Update Arabic, Portuguese, Interlingua, Norwegian translations
- Only enable external audio input when audio track is rec-armed

### Fixed
- Fix range not being drawn properly in ruler when scrolling horizontally

## [0.8.757] - 2020-07-30
### Added
- New dependency: reproc
- Add None output to direct outs
- Add test for finding installed LV2 plugins
- Add test for copying plugins
- Add VST tests

### Changed
- Bump meson version requirement to 0.55.0 (for reproc cmake subproject)
- Use reproc to run processes with timeout instead of glib
- Updated German, Portuguese, Chinese, Japanese, Spanish, French translations
- Change trial period to 30 minutes
- Do soft graph recalculation when plugin latency changes (wait for pause if playing)
- Don't save log contents in memory
- Use GtkSourceView for log viewer
- Use shared lib when linking tests (dramatically improves linking time)
- Don't show backtraces in the log file
- Let children signals pass through when group track is soloed
- Show latencies in exported graphs
- Do not recompute JACK latencies during plugin processing
- Show warning instead of critical when carla plugin discovery fails

### Fixed
- Fix carla rack not being instantiated as an instrument on the track
- Fix arranger selections not being initialized for some actions
- Fix multiple non-fatal errors on windows on first run
- Fix VST scan sometimes hanging on Windows
- Fix crash when muting instrument tracks
- Fix LV2 plugin ports with URI parameter throwing warnings
- Fix child tracks not being disconnected or reconnected when deleting a group track and undoing
- Fix sends not being removed/readded when deleting the target track and undoing
- Fix sends being lost on undo after sending track is deleted
- Fix custom port connections not being restored when undoing track deletion
- Fix Carla plugin UIs not being shown on top
- Fix DSP thread becoming deadlocked when nodes with playback latency exist
- Fix metronome occasionally receiving invalid positions
- Fix recording action not being free'd on exit (causing warnings)
- Fix issue when loading project with Noisemaker VST
- Fix direct outs not being updated properly when moving/duplicating/deleting tracks
- Fix playback latencies not being calculated properly
- Fix number of frames miscalculation when starting playback when playback latencies exist
- Fix duplicated plugin not being activated
- Fix plugin identifier not being taken into account when searching for automation tracks from a port
- Fix piano roll port not being connected to track processor in the graph

## [0.8.694] - 2020-07-18
### Added
- Allow routing from chord track to instrument tracks
- Integration test for creating/deleting tracks using Helm
- Add additional checks when tracks are added to the project
- Add shift-selection for selecting multiple tracks or channels
- Add option for level of UI detail (improves CPU usage on lower end machines)
- Show bug report dialog on non-fatal errors
- Add tests for creating plugins and port connection actions
- Make port connections and channel sends undoable
- Show error if icon is not found at startup
- Add authors to credits section in the manual
- Add Guile API for creating sends between tracks, connecting ports between a plugin and a track, and creating tracks as undoable actions
- Add Guile test runner
- Add Trademark Policy for Zrythm wordmark and logo
- Add option to override the program name

### Changed
- Refactor: add `is_project` to many objects
- Use weakjack on Windows
- Add version requirement on RtAudio dependency
- Only create MIDI notes on track 1 when exporting a MIDI region
- Update German, Galician, Japanese translations
- Print function name and line number in the log
- Open plugins that require the KX UI interface with carla
- Various drawing optimizations (by passing integers to cairo)
- Use configuration file for tests
- Each plugin instance now has its own state directory, including non-project plugin instances
- Save plugin states when plugin instances are created
- Ignore sysex messages from LV2 plugins for now
- Update Guile API docs with more examples
- Catch invalid SFZ/SF2 paths instead of crashing
- Add detailed license information for each icon

### Fixed
- Fix crash when undoing twice after deleting a track
- Fix crash when creating a new plugin fails
- Fix MIDI note offs not being sent at the right time when moving MIDI notes
- Fix issues with initialization of undoable actions when loading projects
- Fix crash when closing Zrythm after resizing an automation region
- Fix automation tracks not being cloned properly
- Fix crash when undoing track deletion with automation
- Fix automation regions not properly duplicated when duplicating tracks
- Fix MIDI file import on Windows
- Fix loading new projects from a loaded project
- Fix an issue with exporting
- Fix crash when connecting a plugin CV out port to another track's balance control
- Fix automation track ID track positions not being updated when moving plugins from one track to another
- Fix passing a project file as a command line argument not working
- Fix crash when moving tracks
- Fix editor not being refreshed when region owner track is deleted

## [0.8.604] - 2020-06-26
### Added
- Add tempo track for BPM automation
- Time-stretch audio regions in musical mode in real-time
- Add musical mode selection to region context menu
- Add --gdb, --cachegrind, --midi-backend, --audio-backend, --dummy, --buf-size and --interactive command line options
- Add plugin browser context menu to open plugins with carla

### Changed
- Move mute and solo ports from track to fader
- Add initial processor to routing graph
- Routing code refactor: split to smaller files and simplify
- ZrythmApp refactor: split code to Zrythm and ZrythmApp
- Events code refactor: split to Event and EventManager
- Other refactoring to allocate/free resources properly
- Update French, Portuguese, Norwegian translations
- Draw musical mode icon on regions
- Disable region draw caching
- Use cyaml v1.1.0 or above for precise float/double serialization
- UI: make channel slots and sends shorter
- Allow multiple Zrythm instances to be opened
- Use the same naming scheme as native plugin UIs for carla plugin UI titles
- Audio regions are now created at the playhead when creating new tracks
- Use `get_state`/`set_state` for saving/loading the plugin state for carla plugins

### Fixed
- Fix editor quantization
- Fix various memory leaks
- Fix MIDI file export
- Fix mute/solo not affecting the fader
- Fix audio recording
- Fix undo stack initialization when loading projects
- Fix initialization of recorded regions on project load
- Fix issues when resizing audio regions
- Fix crash when cutting regions
- Fix MIDI note positions being global instead of local during recording
- Fix crash when undoing after using the eraser
- Fix double instantiation of plugins when adding new tracks
- Fix range selection
- Fix crash when deleting a chord region

## [0.8.535] - 2020-06-06
### Added
- Add image-missing fallback if icon is not found
- Add build flag for extra optimizations
- Show red reading in meters if peak above 0db, grey otherwise
- Show native CPU usage meter on MacOS
- Show build type in `--version`
- Add VCS tag fallback version if git is not found
- Make BPM changes undoable
- Make changing the automation curve algorithm undoable
- Show chord notes in chord editor lanes
- Show color gradient in meters

### Changed
- Fallback to older compression API if zstd version is lower than 1.3
- Don't attempt to build manpage on windows
- Update French, Portuguese, Galician translations
- Log to stderr until log file is initialized
- Ask for LV2 plugin latency by passing block length instead of 2 samples
- Underscorify and simplify build flags
- Don't allow multiple preferences windows
- Use LINGUAS file to determine available locales for meson
- Open KX external UIs with carla
- Make markers, chords and scales rounded rectangles
- Use peak meter in mixer channels and RMS meter for master with falloffs (algorithms from x42 meters)
- Chord editor now has 12 fixed chord slots
- Accents in the chord creator are now toggled on single click
- Only resize audio regions when BPM change is completed (in musical mode)
- Use MPMC queue instead of a stack for queueing up objects to be deleted with `free_later()`

### Deprecated
- Deprecate libgtop support

### Fixed
- Fix zstd library discovery
- Fix setting integers on gsettings keys expecting booleans
- Fix CPU meter on windows
- Fix various crashes with chord editor
- Fix freezes when using RtMidi on Windows
- Fix various MacOS issues
- Fix scales not saved properly
- Fix moving tracks clearing regions of tracks being moved
- Fix undo/redo when moving tracks
- Fix MIDI note on incorrectly being fired at transport loop point

## [0.8.459] - 2020-05-15
### Added
- Add real time display to playhead meter
- Add error handling when project fails to load
- Add missing actions in automation editor
- Add undoable region stretching and related cursors
- Add option to return to cue point on stop
- Add integration test for loading and playing back MIDI files
- Add CLI commands to convert between .zpj and .yaml

### Changed
- Don't attempt to draw fades if there are none
- Huge update of the user manual
- Update French, Japanese translations
- Use a semaphore to lock port operations during do/undo
- Make `free_later()` traverse the stack in a non-GTK thread to avoid UI freezes when many objects are free'd
- Bridge all GTK/Qt plugin UIs with carla
- Auto-generate list of translators for about dialog from the TRANSLATORS file
- Cache drawing of audio regions to prevent freezes when multiple regions are on the screen
- Update build instructions for Windows
- Use zstd to compress Zrythm project files

### Removed
- Remove cached positions from arranger objects
- Remove breeze icons from distribution (now a runtime dependency)

### Fixed
- Fix sends being lost when moving tracks
- Fix extension not properly extracted for filenames with multiple dots
- Fix VST scanning in user home dir
- Fix gcc 10 warnings/errors
- Fix various issues with LV2 plugins loaded through carla
- Fix bounce on tracks with carla plugins
- Fix MIDI note off occasionally ignored
- Fix snap points not being updated when BPM changes
- Fix build with RtAudio but without RtMidi
- Fix rtaudio input devices with the same name being string-matched as the selected output device

## [0.8.397] - 2020-05-05
### Added
- Install freedesktop-compliant icon theme
- Allow icon theme customization through user overrides
- Preset/bank selector in plugin inspector

### Changed
- For unsupported UIs, bridge only UI if possible, otherwise bridge whole plugin
- Use comma to separate paths on windows
- Rename meson options to use dash instead of underscore
- Don't use jack2-only API

### Fixed
- Fix build with `user_manual` option
- Fix deleting midi notes removing other notes
- Fix balance control reset not working

## [0.8.378] - 2020-05-01
### Added
- Add AU, VST2, VST3 support for MacOS
- Add SFZ and SF2 support
- Option to bridge unsupported UIs as an external process

### Changed
- Automation curves are now grabbable only if the cursor is near them
- Project backups are now saved asynchronously
- Show localized strings with `-p --pretty`
- Show switches instead of checkboxes for toggle ports in generic UIs
- Use Guile to generate gsettings schema
- Use meson features instead of booleans for optional dependencies
- Generate preferences window automatically
- Use system locale on first run
- Change default Zrythm path to XDG data home specification
- Change libcyaml dependency version to allow newer versions
- Update Japanese, Galician, German, Spanish, Chinese translations

### Fixed
- Fix using incorrect paths on Windows/MacOS
- Fix moving multiple automation points setting them all to the same value
- Fix deleting instrument track not deleting instrument UIs
- Fix MIDI FX ports not being added to routing graph
- Fix crash on systems with > 16 CPU cores
- Fix generic UI windows being shrinkable beyond control visibility

## [0.8.333] - 2020-04-26
### Added
- Add MIDI FX section in track inspector
- Add comment field in track inspector (WIP)
- Remember reveal status of track inspector sections
- Zrythm is now user themable on XDG base dir-compliant systems (`$XDG_CONFIG_HOME/zrythm/theme.css`)
- Add button to export routing graph as image or dot graph
- Add MIDI pitch bend and modwheel automatables to MIDI-capable tracks

### Changed
- Make gtksourceview mandatory
- Instruments go on a special slot instread of in the inserts
- Don't allow plugins with non-matching output type in channel slots
- Channel sends now work as single stereo/MIDI bundle instead of separate ports
- Zrythm theme is now installed externally instead of bundled as a resource
- Bug reports now include last few lines of log
- Process automation when the control port is processed instead of collectively at the start of each cycle

### Deprecated
- Deprecate VST support without carla

### Fixed
- Fix VST plugins not scanned in distros whose libdir name is not `lib`
- Fix first MIDI note ignored on Windows during export
- Fix edit cursor positioning

## [0.8.298] - 2020-04-18
- Add full vst2 support via carla (32bit and 64 bit)
- Add experimental vst3 support via carla (windows only)
- Show inserts in track inspector
- Add fader section in track inspector
- Re-layout channel widgets
- Add style scheme for scripting interface
- Split status bar info to 2 lines
- Show currently scanned plugin in splash screen
- Update Galician translations

## [0.8.252] - 2020-04-10
- New layout: move transport bar to the bottom
- Split inspector stack pages to separate notebook pages
- Add plugin bypass option
- Add region linking
- Add region/midi note muting
- Implement cut-paste for arranger objects
- Add script examples to manual
- Expose midi track/region/midi note creation to guile interface
- Use GtkSourceView for scripting interface
- Show exception and backtrace when script execution fails
- Add cut/copy/duplicate/delete options to arranger context menus
- Make channel slots shorter
- Update Portuguese, German, Norwegian translations
- Fix installation failure when using `enable_manual`
- Fix FX track being created instead of MIDI when MIDI plugin is instantiated
- Fix copy-paste not working
- Fix function snarfing for guile interface

## [0.8.200] - 2020-03-30
- Show peaks in meters
- Show units in generic UIs for LV2 plugins when available
- Show track control ports in the inspector
- Add automation recording functionality
- Add scripting functionality with guile
- Add guile API docs to the manual
- Convert some scripts from python to guile
- Update Galician, Russian translations
- Fix calculation for logarithmic ports
- Fix mute status not showing on the track

## [0.8.156] - 2020-03-19
- Add option to bounce material to audio
- Add cability to fade in/out audio regions
- Add capabilities to version info in bug report links
- Implement ctrl + arrow keys movement in arrangers
- Run work from LV2 workers immediately when freewheeling
- Show description in `--print-settings`
- Add `--pretty` option for colored output
- Add various curve algorithms selectable via right click
- Use check menu items instead of stars to indicate current selection
- Don't allow removing the last visible automation track
- Use boolean instead of int for booleans in gsettings
- Add tests for curve algorithms
- Use `GSETTINGS_SCHEMA_DIR` in the desktop file
- Make gschema translatable
- Update French, Galician translations
- Fix crash when attempting to load MP3 files when built without ffmpeg
- Fix crash when opening preferences when the selected backends are not compiled into the current version
- Fix automation curves being cut off at the end
- Fix crash when moving marker track
- Fix solo buttons

## [0.8.113] - 2020-03-11
- Show capabilities with `--version`
- Write queued logs periodically instead of on every UI cycle
- Add `--reset-to-factory` and `--print-settings` options
- Add bash completion script
- Add more info to the manpage
- Implement ctrl+D for duplicating events
- Allow movement of events with arrow keys
- Add all languages to the language selection with native spelling
- Make undo history serializable and unlimited (off by default)
- Add sub-tick precision to positions
- Add eg-fifths to plugin tests
- Add tests for saving/loading projects
- Added Galician translation
- Update Japanese translations
- Fix crash on project load

## [0.8.038] - 2020-03-04
- Add RtAudio support
- MacOS release

## [0.8.001] - 2020-02-28
- Show plugin slot in the window name
- Only show 1 XRUN message per 6 seconds
- Allow using shift to set more precise values in faders and balance widgets
- Add log viewer
- Only write logs in the GTK thread (possibly fixes occasional XRUNS)
- Don't allow JACK MIDI to be selected with non-JACK audio backend
- Bring back original file browser
- Update splash screen and about menu graphics
- Add export test
- New Spanish translations
- Fix track positions on other tracks not getting updated after deleting a track
- Fix routing graph not being recalculated after loading a project
- Fix automation tracks getting duplicated on project load
- Fix build failure with -fno-common
- Fix markers not being renamable

## [0.7.573] - 2020-02-21
- Add option to passthrough MIDI channel on MIDI tracks and lanes
- Add delete button in port connections
- Add hack for showing a dark window behind plugin windows on Windows (temporary solution for plugins with transparent backgrounds)
- Add port info and object info dialogs
- Refactor arranger objects to use indices instead of pointers
- Add ability to double-click on tracks to rename
- Add tests for audio regions and moving regions to other tracks
- Add tests for plugin operations
- Copy/move automation to the new track if plugin is moved/cloned
- Allow moving plugins to empty space to create a new track
- Show port name instead of port symbol in the inspector for LV2 plugins
- Only send time info to LV2 plugins that want it
- Only serialize VST descriptors at the end of the scan instead of after each scanned plugin
- Support LV2 trigger, toggle, notOnGui and integer port properties
- Connection multipliers now apply to audio and CV ports too
- Recording audio or MIDI data is now undoable
- Convert channel mute, volume and stereo balance to automatable control ports
- Remove bundled libcyaml and use a meson wrap instead
- Fix crash when clicking Add to add a port connection
- Fix crash when handling state files for plugins with illegal chars in their name
- Fix audio regions getting resized by a few samples when moved
- Fix undo causing a crash after importing MIDI files
- Fix live waveform display showing the opposite values (upside down)
- Fix ctrl+clicking unselected objects not selecting them
- Fix crash when loading a project with duplicated tracks
- Fix incorrectly allowing plugin ports to connect to ports of the same plugin
- Fix bug with hiding and showing tracks
- Fix missing icon on GNU/Linux and FreeBSD
- Fix LV2 plugin support on Windows
- Fix arranger objects getting deleted when deleting a plugin from the mixer
- Fix original regions not getting shown when ctrl-dragged regions move out of sight
- Fix LV2 plugins not getting scanned properly on Ubuntu 19.10
- Fix crash when metronome samples are not found
- Fix channel direct outs not getting stored properly
- Fix support for LV2 external UIs
- Fix crash when selecting audio input
- Update Spanish, Japanese, French translations

## [0.7.474] - 2020-01-27
- Add MIDI file import
- Add timeout when scanning for VST plugins
- Add VST2 path selection for Windows
- Add samplerate, device and buffer size options for SDL2 backend
- Add flag to compile a trial version
- Fix VST preset changes not remembered during saving
- Don't scan for plugins before the welcome dialog has been completed
- Fix piano roll remaining opened when its region's track is deleted
- Fix Windows MME MIDI events getting dropped if they are late
- Fix crash when creating an audio track when more than 60 audio inputs exist
- Fix crash when loading projects with audio files
- Fix audio track duplication warnings
- Fix autosave not working
- Fix crash when selecting Windows MME device in track inputs
- Fix crash when dragging plugins in mixer after loading a project
- Add editing info to the manual
- Update French, Japanese translations

## [0.7.425] - 2020-01-23
- Add undoable plugin delete action
- Send time info to VST plugins
- Make VST parameters editable from the inspector
- Make VST parameters automatable
- Hardcode standard LV2 and VST paths
- Start logging earlier during initialization
- Add English UK translation
- Update Chinese and Japanese translations
- Fix VST plugin window not getting closed when plugin is deleted
- Fix crash when cloning VST plugins
- Fix window and exe icons not showing on Windows
- Fix KX studio LV2 UIs crashing on load
- Fix VST scanning not working on Windows
- Fix localization not working on Windows
- Fix broken WAV export
- Fix VSTs not receiving correct sample rate and block length
- Fix fullscreen action

## [0.7.383] - 2020-01-18
- Add SDL2 audio backend
- Add experimental Windows VST support
- Close VSTs properly before freeing the library
- Fix non-instrument VST plugins being shown as instruments
- Fix cached VSTs getting ignored
- Fix Windows MME MIDI timings
- Fix unavailable backends shown as available
- Fix buffer overflow when more than 16 hardware audio ports exist
- Fix metronome playback being triggered at negative offsets causing crashes

## [0.7.367] - 2020-01-13
- Update Zrythm logo
- Wrap backtrace on crash inside scrolled window
- Convert MIDI channel of input events to the track's channel when playing live
- Add VST descriptor caching to make scanning faster
- Add VST blacklist for plugins that fail to load
- Fix some memory leaks
- Fix audio and MIDI backend selection using wrong values in different platforms

## [0.7.345] - 2020-01-05
- Add experimental VST2 support (X11 UIs only)
- Previously visible plugins now open on project load
- Fader and pan controls are now exposed as ports
- Add option to listen to notes while dragging them in the piano roll
- Hide uneditable port connections from the inspector
- Show visible track count above the track list
- Allow vertical resizing of piano roll
- Use lilv subproject
- Made black rows darker in piano roll
- Allow older GTK versions (works on Debian 9)
- Fix Windows build
- Fix using symbolically linked icons in compiled resources
- Updated French and Portuguese translations
- Add Greek translation to the manual

## [0.7.295] - 2019-12-24
- Add toggle for musical mode
- Add separators in timeline toolbar
- Send control change events to plugin UIs when automating with CV (UIs now get refreshed properly)
- Fix crash when opening a project created with a different backend
- Fix a few functions returning invalid memory
- Fix crash during graph validation when connecting ports
- Fix UI events wrongly being fired after the Zrythm window closes (causing mini-crashes on exit)
- Fix crash when resizing automation regions
- Fix timeline minimap incorrectly changing the editor zoom level
- Fix crashes during audio recording
- Fix toggling visibility of panels not working
- Fix broken randomization slider in the quantize dialog
- Set minimum required meson version
- Add xdg-utils dependency
- Update French, Portuguese and German translations

## [0.7.270] - 2019-12-21
- Make Zrythm icon more symmetric
- Auto-stretch audio regions when changing BPM
- Make position calculations more accurate
- Handle recording in the GUI thread instead of the RT thread
- Don't autosave during playback
- Fix automatable selectors not working in automation tracks
- Fix automation track add and remove buttons
- Fix audio regions not being drawn
- Fix audio arranger not getting its full size
- Fix balance controls in the mixer incorrectly attennuating stereo signals
- Fix monitor out knob not working
- Update user manual
- Update French, Japanese, German and Portuguese translations
- Remove unnecessary qt5 dependency
- Remove unused source files

## [0.7.186] - 2019-12-12
- Major rework of arrangers (arrangers are now canvases)
- Major rework of how tracks are drawn (single canvases)
- Drawing optimizations for arrangers and other widgets
- Refactor of arranger object widget code
- Refactor arranger objects to make operations simpler
- Use an MPMC queue with an object pool for UI events to make it RT-safe
- Make automation points prettier
- Use libaudec for reading and resampling audio files
- Add ability to resize individual track lanes
- Add bundled plugin ZLFO
- Add option to use system fonts or bundled
- Use RT-safe ring buffer for remembering audio and MIDI data from the RT thread
- Add output meters to tracks
- Optimize some math calculations
- Add sr.ht builds
- Remove libdazzle docks
- Fix build issues with Gentoo and FreeBSD
- Fix lag during playback when many objects are on the screen
- Fix random crash and glib warnings on startup
- Fix plugins with CV ports not getting serialized properly
- Fix license headers on some files incorporating work from other projects
- Fix mixer meters occasionally blinking to -inf
- Various other bugfixes

## [0.7.093] - 2019-11-03
- Add ability to pin and unpin tracks
- Allow copy paste of MIDI notes and chords
- Allow creating a new project while Zrythm is running
- Allow binding of MIDI CC messages to controls
- Add some tests for arranger selection actions
- Add meson option to run stoat
- Change port buttons to sliders (editable for controls)
- Add detailed tooltips for ports in the inspector
- Make GUI tests optional when enabling tests
- Make RMS calculations more accurate
- Allow to set position from the editor
- Show local position in the editor during playback
- Huge code refactor of arranger objects
- Automation curves are now merged into automation points
- Pass control change events to LV2 plugin UIs when changing from inside Zrythm
- Refactor of region drawing code
- Refactor of lv2 plugin code
- Fix generic UIs not getting updated when control values change during automation
- Fix auto-connecting to MIDI devices
- Fix crash when zooming in too much with regions visible (MIDI regions only for now)
- Fix various bugs with chord objects
- Fix crash when clicking in empty tracklist
- Fix crash when saving a project after cloning a track
- Fix curves not getting redrawn when moving automation points
- Update Portuguese, French, Arabic, Polish, German translations

## [0.7.021] - 2019-10-18
- Add fishbowl benchmark tests for widgets
- Optimize and cache various widgets
- Make clicking on plugins in the mixer more responsive
- Add event viewers for the timeline and editors
- Fix channel slot names not displaying
- Fix track routing getting ignored when record is not armed in the targe track
- Update Japanese translations

## [0.7.002] - 2019-10-14
- Use a lock-free queue while processing instead of a mutex
- Add caching where pango is used to draw text
- Add `ZRYTHM_DSP_THREADS`, `NO_SCAN_PLUGINS` and `ZRYTHM_DEBUG` environment variables
- Add .zpx and .zpj extensions for Zrythm projects and packages
- Add option (on by default) to keep plugin windows on top
- Move file browser to popup window instead of right panel
- Show plugin scan progress
- Make panel icons larger
- Change plugin window titles to show track name, plugin name and preset, if any
- Various optimizations
- Add some missing dialog icons
- Add additional checks in the project assistant
- Export in chunks when exporting the project to audio
- Fix MIDI note staying on after region ends
- Fix crash when no hardware ports found
- Fix error when project list is empty
- Fix panels getting weird positions when hiding and re-showing
- Fix various memory leaks
- Fix osx build
- Fix crash when clicking piano roll key labels
- Fix some crashes when duplicating tracks
- Fix graph getting recalculated twice when clicking on a region
- Fix crash when trying to add a port connection
- Fix grab broken warnings when clicking on a MIDI note after project load
- Set GTK dependency to 3.24
- Clean up log messages
- Make `config_h` a meson dependency to ensure it gets generated before building sources
- Add tests for MIDI track and position

## [0.6.502] - 2019-10-08
- Add caching for drawing automation regions
- Allow direct routing from midi track to instrument track
- Move issue tracker to Redmine
- Send MIDI note offs 1 sample before the loop/region ends
- Add additional checks when connecting monitor outs to JACK
- Move MIDI channel selectors for tracks and lanes in context menus
- Updated to new libcyaml (allow-null feature)
- Fix pan widget not setting the right values
- Fix moving of tracks to not skip space in-between or space after the last track
- Fix MIDI note on being sent at the end point of the region
- Fix MIDI note still playing if its end point is after loop end
- Fix qsort comparison regression
- Fix various automation-related bugs
- Fix piano roll keys not sending correct MIDI channel
- Fix various chord object-related actions
- Fix chord regions not getting loaded properly
- Fix plugin slot not getting serialized for LV2 port IDs
- Fix warning messages while playing back MIDI regions
- Minor refactoring

## [0.6.480] - 2019-10-01
- Add audio recording functionality
- Add track processor for processing track input ports and recording
- Add input selectors for audio tracks
- Add global spacebar key binding for play and pause
- Add warranty disclaimer on first run
- Add warning in the Preferences when selecting a language for which a locale is not installed
- Add News button that points to the CHANGELOG
- Go to previous marker if backspace is pressed twice
- Allow undoable moving and copy-moving of regions to other tracks and other lanes
- Move panel toggles to View toolbar
- Only allow tracks with audio outs to be routed to Master
- Fix audio clip import and playback
- Fix MIDI recording
- Fix record not getting disarmed when clicking on another track
- Fix mute/solo not working properly
- Fix splash screen problems in ubuntu
- Create hard links for identical files in the user manual
- Remove unused x42 plugins from distribution
- Update French, German, Japanese, Portuguese translations

## [0.6.422] - 2019-09-19
- Add ability to select MIDI channel for the piano roll per track and per lane
- Add direct out in the track properties
- Add audio pool (WIP)
- Use GtkFileBrowserWidget for the file browser
- Add optimization flags for release builds
- Add stricter compilation flags for normal builds
- Fix song export
- Fix loading backup projects containing plugins
- Fix loading a project while Zrythm is already running
- Fix allowing to select projects that don't exist in the project assistant
- Add additional checks for the pre-release scripts
- Use installed suil if available instead of local one
- Update parts of the documentation
- Update French, German, Portuguese translations

## [0.6.384] - 2019-09-14
- Switch from AGPL to GFDL for the user manual
- Add option to resize regions normally or to resize-loop
- Add warning when loading a project made with a different version of Zrythm
- When creating a new project, immediately save it
- Add check for failure in the selected backends when launching Zrythm and revert to dummy backends
- Print the actual MIDI message when an unknown message is received
- Append a number to the filename to be exported if a file already exists
- Fix piano roll key press events not getting propagated to the channel
- Fix moving notes emitting unnecessary MIDI events
- Fix basic copy-paste in the timeline
- Fix regions not getting moved to the new track after moving track position
- Fix loop selection not taking into account the end position of the last selected object
- Fix bar 1 not getting drawn in the ruler
- Fix split cursor remaining enabled after alt tabbing
- Change region resize behavior when moving the left edge to add space instead of move the items
- Add MIDI bus and MIDI group tracks
- Update the user manual
- Update German, Portuguese, French, Japanese, Norwegian translations

## [0.6.323] - 2019-09-07
- Add autosave (backup) mechanism
- Add support for project templates
- Allow exporting whole project as MIDI
- Add functionality to open the directory after exporting
- Add popup to choose to load the backup if it is newer than the loaded project
- Add dialog for choosing parent directory and name when creating new projects
- Move panel toggles to the top of zrythm
- Fix requiring GTK 3.24 for no good reason
- Update German, Portuguese, Norwegian, Polish, French, Japanese translations

## [0.6.261] - 2019-09-05
- Add alt and click to cut region in half
- Allow further zooming in and out
- Show sixteenths in the ruler when zoomed
- Changed status bar to show audio engine info instead of tips
- Add song start and end markers
- Remember left and right panel divider positions
- Add Zrythm path to preferences dialog
- Add export progress dialog
- Install mime type for Zrythm projects
- Fix saving and loading of simple projects
- Fix piano roll not getting full height
- Fix some arrangers not getting their full width when zoomed
- Fix tracks getting deselected when clicking on empty space
- Remove project info screen from the startup dialog
- Refactoring of arranger objects
- Fix various minor bugs
- Fix some incorrect and unclear license notices
- Updated Portuguese, Italian, German, French, Norwegian translations

## [0.6.175] - 2019-09-01
- Piano roll now opens centered on middle C
- Add context menu option for exporting MIDI regions into MIDI files
- Add track visibility panel and option to hide track in the track context menu
- Make track name editable by double clicking on it in the track
- Add ability to move tracks inside the tracklist
- Double clicking on regions now brings up the bottom panel if hidden
- Make MIDI track functional
- The monitor fader in the control room section is now operational and the level is persisted
- Add inputs section in each recordable track to choose input device for recording
- Add input MIDI channel filter on MIDI and instrument tracks
- Add meson target for collecting translatables
- Use track/plugin/port or track/port designation for ports
- Ellipsize text in various places to prevent overflow of widgets
- Change donate dialog to direct link to LiberaPay
- Add some missing tooltips
- Show selected track info (master by default) on startup
- User manual button now opens manual locally if installed
- Add ability to scroll with mousewheel to change values in meters
- Change chatroom link to point to the new chatroom
- Add meson version dependency
- Grid now adapts to zoom level
- Bug report link now prefills the report
- Use git version and commit hash as the version string if available
- Fix piano roll key shade not going away and not being able to click and hold to play multiple keys
- Fix JACK ports not getting renamed after a track gets renamed
- Fix splash screen and lock track icons not showing
- Fix auto-scroll in piano roll
- Fix piano roll events being sent to channel 2 instead of channel 1
- Fix some regressions
- Arranger object refactoring and minor fixes
- Signal flow refactoring and bugfixes
- Updated Japanese, French, Portuguese, German translations

## [0.6.039] - 2019-08-21
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
