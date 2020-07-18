# Changelog
All notable changes to this project will be documented in this file.

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

----

Copyright (C) 2019-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
