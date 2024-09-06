# SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import xml.dom.minidom
import xml.etree.ElementTree as ET
import sys

output_file = sys.argv[1]
app_id = sys.argv[2]
linguas = sys.argv[3]

root = ET.Element("schemalist")
root.set ("gettext-domain", "zrythm")
copyright_text = ET.Comment (
    """
    SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
    SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
    SPDX-License-Identifier: LicenseRef-ZrythmLicense
    """
)
root.append (copyright_text)

def print_enum(id, nicks):
    el = ET.SubElement(root, "enum")
    el.set("id", app_id + "." + id + "-enum")
    for i, nick in enumerate(nicks):
        value_el = ET.SubElement(el, "value")
        value_el.set("nick", nick)
        value_el.set("value", i.__str__())

print_enum("audio-backend", ["none", "none-libsoundio", "alsa",
             "alsa-libsoundio", "alsa-rtaudio", "jack",
             "jack-libsoundio", "jack-rtaudio",
             "pulseaudio",
             "pulseaudio-libsoundio",
             "pulseaudio-rtaudio",
             "coreaudio-libsoundio", "coreaudio-rtaudio",
             "sdl", "wasapi-libsoundio",
             "wasapi-rtaudio", "asio-rtaudio"])
print_enum("midi-backend", ["none", "alsa", "alsa-rtmidi", "jack",
            "jack-rtmidi", "wmme", "wmme-rtmidi",
            "coremidi-rtmidi"])
print_enum("language", linguas.split(" "))
print_enum("export-time-range", ["loop", "song", "custom"])
print_enum("export-filename-pattern", ["append-format",
            "prepend-date-append-format"])
print_enum("note-length", ["bar", "beat", "2/1", "1/1", "1/2", "1/4",
            "1/8", "1/16", "1/32", "1/64", "1/128"])
print_enum("note-type", ["normal", "dotted", "triplet"])
print_enum("midi-modifier", ["velocity", "aftertouch"])
print_enum("note-notation", ["musical", "pitch"])
print_enum("plugin-browser-tab", ["collection", "author", "category",
            "protocol"])
print_enum("plugin-browser-filter", ["none", "instrument", "effect", "modulator",
            "midi-effect"])
print_enum("plugin-browser-sort-style", ["alpha", "last-used", "most-used"])
print_enum("file-browser-filter", ["none", "audio", "midi", "preset"])
print_enum("piano-roll-highlight", ["none", "chord", "scale", "both"])
print_enum("pan-law", ["zero-db", "minus-three-db",
            "minus-six-db"])
print_enum("pan-algorithm", ["linear", "sqrt", "sine"])
print_enum("curve-algorithm", ["exponent", "superellipse", "vital", "pulse", "logarithmic"])
print_enum("buffer-size", ["16", "32", "64", "128", "256", "512", "1024",
            "2048", "4096"])
print_enum("sample-rate", ["22050", "32000", "44100", "48000", "88200",
            "96000", "192000"])
print_enum("transport-display", ["bbt", "time"])
print_enum("jack-transport-type", ["master", "client", "none"])
print_enum("tool", ["select-normal", "select-stretch",
            "edit", "cut", "erase", "ramp",
            "audition"])
print_enum("graphic-detail", ["high", "normal", "low", "ultra-low"])
print_enum("recording-mode", ["overwrite-events", "merge-events",
            "takes", "muted-takes"])
print_enum("bounce-step", ["before-inserts", "pre-fader",
            "post-fader"])
print_enum("preroll-count", ["none", "one-bar", "two-bars",
            "four-bars"])
print_enum("default-velocity", ["last-note", "40", "90", "120"])

def make_schema_key(parent_el, name, type, default, summary, description):
    el = ET.SubElement(parent_el, "key")
    el.set("name", name)
    el.set("type", type)
    default_el = ET.SubElement(el, "default")
    default_el.text = ("'" + default + "'") if (type == 's') else default
    summary_el = ET.SubElement(el, "summary")
    summary_el.text = summary
    description_el = ET.SubElement(el, "description")
    description_el.text = description
    return el

def make_schema_key_with_range(parent_el, name, type, range_min, range_max, default, summary, description):
    el = make_schema_key(parent_el, name, type, default, summary, description)
    range_el = ET.SubElement(el, "range")
    range_el.set("min", range_min)
    range_el.set("max", range_max)
    return el

def make_schema_key_with_enum(parent_el, name, enum, default, summary, description):
    el = ET.SubElement(parent_el, "key")
    el.set("name", name)
    el.set("enum", app_id + "." + enum + "-enum")
    default_el = ET.SubElement(el, "default")
    default_el.text = "'" + default + "'"
    summary_el = ET.SubElement(el, "summary")
    summary_el.text = summary
    description_el = ET.SubElement(el, "description")
    description_el.text = description
    return el

def get_schema_path(schema):
    return "/" + schema.replace(".", "/") + "/"

def make_schema(parent_el, local_id):
    el = ET.SubElement(parent_el, "schema")
    full_id = app_id + "." + local_id
    el.set("id", full_id)
    el.set("path", get_schema_path(full_id))
    return el

def make_preferences_schema(local_id):
    el = make_schema(root, "preferences." + local_id)
    return el

# General
general_schema = make_schema(root, "general")
make_schema_key(general_schema, "recent-projects", "as", "[]", "Recent project list", "A list of recent projects to be referenced on startup.")
make_schema_key(general_schema, "first-run", "b", "true", "First run", "Whether this is the first time the app is run.")
make_schema_key(general_schema, "install-dir", "s", "", "Installation directory", "This is the directory Zrythm is installed in. Currently only used on Windows.")
make_schema_key(general_schema, "last-project-dir", "s", "", "Last project directory", "Last directory a project was created in.")
make_schema_key(general_schema, "run-versions", "as", "[]", "List of all versions run at least once", "A list of versions run at least once.")
make_schema_key(general_schema, "last-version-new-release-notified-on", "s", "999", "Last version that was notified of a new release", "The last version that received a 'new version has been released' notification.")
make_schema_key(general_schema, "first-check-for-updates", "b", "true", "First check for updates", "Whether this is the first time attempting to check for updates.")

# Monitor
monitor_schema = make_schema(root, "monitor")
make_schema_key_with_range(monitor_schema, "monitor-vol", "d", "0.0", "2.0", "1.0", "Monitor volume", "The monitor volume in amplitude (0 to 2).")
make_schema_key_with_range(monitor_schema, "mute-vol", "d", "0.0", "0.5", "0.0", "Mute volume", "The monitor mute volume in amplitude (0 to 2).")
make_schema_key_with_range(monitor_schema, "listen-vol", "d", "0.5", "2.0", "1.0", "Listen volume", "The monitor listen volume in amplitude (0 to 2).")
make_schema_key_with_range(monitor_schema, "dim-vol", "d", "0.0", "0.5", "0.1", "Dim volume", "The monitor dim volume in amplitude (0 to 2).")
make_schema_key(monitor_schema, "mono", "b", "false", "Sum to mono", "Whether to sum the monitor signal to mono.")
make_schema_key(monitor_schema, "dim-output", "b", "false", "Dim output", "Whether to dim the the monitor signal.")
make_schema_key(monitor_schema, "mute", "b", "false", "Mute", "Whether to mute the monitor signal.")
make_schema_key(monitor_schema, "l-devices", "as", "[]", "Left output devices", "A list of output devices to route the left monitor output to.")
make_schema_key(monitor_schema, "r-devices", "as", "[]", "Left output devices", "A list of output devices to route the right monitor output to.")

# UI
ui_schema = make_schema(root, "ui")
make_schema_key(ui_schema, "main-window-width", "i", "640", "Main window width", "Main window width.")
make_schema_key(ui_schema, "main-window-height", "i", "480", "Main window height", "Main window height.")
make_schema_key(ui_schema, "main-window-is-maximized", "b", "true", "Main window is maximized", "Whether the main window is maximized.")
make_schema_key(ui_schema, "main-window-is-fullscreen", "b", "false", "Main window is fullscreened", "Whether the main window is fullscreened.")
make_schema_key(ui_schema, "bounce-with-parents", "b", "true", "Bounce with parents", "Whether to bounce with parent tracks (direct outputs).")
make_schema_key(ui_schema, "disable-after-bounce", "b", "false", "Disable after bounce", "Disable track after bouncing.")
make_schema_key_with_enum(ui_schema, "bounce-step", "bounce-step", "post-fader", "Bounce step", "Step in the processing chain to bounce at.")
make_schema_key(ui_schema, "bounce-tail", "i", "0", "Bounce tail", "Tail to allow when bouncing (for example to catch reverb tails), in milliseconds.")
make_schema_key_with_enum(ui_schema, "timeline-object-length", "note-length", "bar", "Timeline object length", "Default length to use when creating timeline objects.")
make_schema_key_with_enum(ui_schema, "timeline-object-length-type", "note-type", "normal", "Timeline object length type", "Default length type to use when creating timeline objects (normal, dotted, triplet).")
make_schema_key_with_range(ui_schema, "timeline-last-object-length", "d", "1.0", "10000000000.0", "3840.0", "Last timeline object's length", "The length of the last created timeline object (in ticks).")
make_schema_key_with_enum(ui_schema, "editor-object-length", "note-length", "beat", "Editor object length", "Default length to use when creating editor objects.")
make_schema_key_with_enum(ui_schema, "editor-object-length-type", "note-type", "normal", "Editor object length type", "Default length type to use when creating editor objects (normal, dotted, triplet).")
make_schema_key_with_range(ui_schema, "editor-last-object-length", "d", "1.0", "10000000000.0", "480.0", "Last editor object's length", "The length of the last created editor object (in ticks).")
make_schema_key_with_enum(ui_schema, "piano-roll-note-notation", "note-notation", "musical", "Note notation", "The note notation used in the piano roll - MIDI pitch index or musical (C, C#, etc.)")
make_schema_key_with_enum(ui_schema, "piano-roll-default-velocity", "default-velocity", "last-note", "Default velocity", "The default velocity to use when creating new MIDI notes.")
make_schema_key(ui_schema, "piano-roll-last-set-velocity", "i", "90", "Last set velocity", "Last set velocity on a MIDI note.")
make_schema_key(ui_schema, "musical-mode", "b", "false", "Musical mode", "Whether to use musical mode. If this is on, time-stretching will be applied to events so that they match the project BPM. This mostly applies to audio regions.")
make_schema_key(ui_schema, "listen-notes", "b", "true", "Listen to notes while they are moved", "Whether to listen to MIDI notes while dragging them in the piano roll.")
make_schema_key(ui_schema, "ghost-notes", "b", "false", "Ghost notes", "Whether to show notes of other regions in the piano roll.")
make_schema_key_with_enum(ui_schema, "piano-roll-highlight", "piano-roll-highlight", "none", "Piano roll highlight", "Whether to highlight chords, scales, both or none in the piano roll.")
make_schema_key_with_enum(ui_schema, "piano-roll-midi-modifier", "midi-modifier", "velocity", "Piano roll MIDI modifier", "The MIDI modifier to display in the MIDI editor (only velocity is valid at the moment).")
make_schema_key(ui_schema, "show-automation-values", "b", "false", "Show automation values", "Whether to show automation values in the automation editor.")
make_schema_key(ui_schema, "browser-divider-position", "i", "220", "Browser divider position", "Height of the top part of the plugin/file browser.")
make_schema_key(ui_schema, "left-panel-divider-position", "i", "180", "Left panel divider position", "Position of the resize handle of the left panel.")
make_schema_key(ui_schema, "left-panel-tab", "i", "0", "Left panel tab index", "Index of the currently opened left panel tab.")
make_schema_key(ui_schema, "right-panel-divider-position", "i", "180", "Right panel divider position", "Position of the resize handle of the right panel.")
make_schema_key(ui_schema, "right-panel-tab", "i", "0", "Right panel tab index", "Index of the currently opened right panel tab.")
make_schema_key(ui_schema, "bot-panel-divider-position", "i", "180", "Bot panel divider position", "Position of the resize handle of the bot panel.")
make_schema_key(ui_schema, "bot-panel-tab", "i", "0", "Bot panel tab index", "Index of the currently opened bottom panel tab.")
make_schema_key(ui_schema, "timeline-event-viewer-visible", "b", "false", "Timeline event viewer visibility", "Whether the timeline event viewer is visible or not.")
make_schema_key(ui_schema, "editor-event-viewer-visible", "b", "false", "Editor event viewer visibility", "Whether the editor event viewer is visible.")
make_schema_key_with_enum(ui_schema, "transport-display", "transport-display", "bbt", "Playhead display type", "Selected playhead display type (BBT/time).")
make_schema_key_with_enum(ui_schema, "jack-transport-type", "jack-transport-type", "none", "JACK transport type", "Selected JACK transport behavior (master/client/none)")
make_schema_key_with_enum(ui_schema, "ruler-display", "transport-display", "bbt", "Ruler display type", "Selected ruler display type (BBT/time).")
make_schema_key_with_enum(ui_schema, "selected-tool", "tool", "select-normal", "Selected editing tool", "Selected editing tool.")
make_schema_key(ui_schema, "midi-function", "i", "0", "Last used MIDI function", "Last used MIDI function (index corresponding to enum in midi function action).")
make_schema_key(ui_schema, "automation-function", "i", "0", "Last used automation function", "Last used automation function (index corresponding to enum in automation function action).")
make_schema_key(ui_schema, "audio-function", "i", "0", "Last used audio function", "Last used audio function (index corresponding to enum in audio function action).")
make_schema_key_with_range(ui_schema, "audio-function-pitch-shift-ratio", "d", "0.001", "100.0", "1.0", "Last used pitch shift ratio", "Last used pitch shift ratio.")
make_schema_key(ui_schema, "timeline-playhead-scroll-edges", "b", "true", "Playhead scroll edges (timeline)", "Whether to scroll when the playhead reaches the visible edge (timeline).")
make_schema_key(ui_schema, "timeline-playhead-follow", "b", "false", "Follow playhead (timeline)", "Whether to follow the playhead. When this is on, the visible area will stay centered on the playhead (timeline).")
make_schema_key(ui_schema, "editor-playhead-scroll-edges", "b", "true", "Playhead scroll edges (editor)", "Whether to scroll when the playhead reaches the visible edge (editor).")
make_schema_key(ui_schema, "editor-playhead-follow", "b", "false", "Follow playhead (editor)", "Whether to follow the playhead. When this is on, the visible area will stay centered on the playhead (editor).")
make_schema_key(ui_schema, "track-filter-name", "s", "", "Track filter name", "Name to filter tracks with.")
make_schema_key(ui_schema, "track-filter-type", "au", "[]", "Track filter type", "Track types to filter tracks with.")
make_schema_key(ui_schema, "track-filter-show-disabled", "b", "true", "Show disabled tracks", "Whether to show disabled tracks when filtering.")
make_schema_key(ui_schema, "track-autoarm", "b", "true", "Auto-arm tracks", "Whether to automatically arm tracks for recording.")
make_schema_key(ui_schema, "track-autoselect", "b", "true", "Auto-select tracks", "Whether to automatically select tracks when a region beloging to the track is clicked.")

# UI - MIDI functions
ui_midi_functions_crescendo_schema = make_schema(root, "ui.midi-functions.crescendo")
make_schema_key(ui_midi_functions_crescendo_schema, "start-velocity", "i", "1", "Start velocity", "Start velocity.")
make_schema_key(ui_midi_functions_crescendo_schema, "end-velocity", "i", "127", "End velocity", "End velocity.")
make_schema_key_with_enum(ui_midi_functions_crescendo_schema, "curve-algo", "curve-algorithm", "superellipse", "Curve algorithm", "Curve algorithm.")
make_schema_key(ui_midi_functions_crescendo_schema, "curviness", "i", "50", "Curviness", "Curviness percentage.")

ui_midi_functions_flam_schema = make_schema(root, "ui.midi-functions.flam")
make_schema_key(ui_midi_functions_flam_schema, "time", "d", "10", "Time", "Time (ms).")
make_schema_key(ui_midi_functions_flam_schema, "velocity", "i", "127", "Velocity", "Velocity of the added note.")

ui_midi_functions_strum_schema = make_schema(root, "ui.midi-functions.strum")
make_schema_key(ui_midi_functions_strum_schema, "ascending", "b", "true", "Ascending", "Whether to strum in ascending order.")
make_schema_key_with_enum(ui_midi_functions_strum_schema, "curve-algo", "curve-algorithm", "superellipse", "Curve algorithm", "Curve algorithm.")
make_schema_key(ui_midi_functions_strum_schema, "curviness", "i", "50", "Curviness", "Curviness percentage.")
make_schema_key(ui_midi_functions_strum_schema, "total-time", "d", "50.0", "Total time", "Total time per chord in ms.")

# UI - Plugin Browser
ui_plugin_browser_schema = make_schema(root, "ui.plugin-browser")
make_schema_key_with_enum(ui_plugin_browser_schema, "plugin-browser-tab", "plugin-browser-tab", "category", "Plugin browser tab", "Selected plugin browser tab.")
make_schema_key_with_enum(ui_plugin_browser_schema, "plugin-browser-filter", "plugin-browser-filter", "none", "Plugin browser filter", "Selected plugin browser filter")
make_schema_key_with_enum(ui_plugin_browser_schema, "plugin-browser-sort-style", "plugin-browser-sort-style", "alpha", "Plugin browser sort style", "Selected plugin browser sort style")
make_schema_key(ui_plugin_browser_schema, "plugin-browser-collections", "as", "[]", "Plugin browser collections", "Tree paths of the selected collections in the plugin browser.")
make_schema_key(ui_plugin_browser_schema, "plugin-browser-authors", "as", "[]", "Plugin browser authors", "Tree paths of the selected authors in the plugin browser.")
make_schema_key(ui_plugin_browser_schema, "plugin-browser-categories", "as", "[]", "Plugin browser categories", "Tree paths of the selected categories in the plugin browser.")
make_schema_key(ui_plugin_browser_schema, "plugin-browser-protocols", "as", "[]", "Plugin browser protocols", "Tree paths of the selected protocols in the plugin browser.")

# UI - File Browser
ui_file_browser_schema = make_schema(root, "ui.file-browser")
make_schema_key(ui_file_browser_schema, "file-browser-bookmarks", "as", "[]", "File browser bookmarks", "Location bookmarks for file browser (absolute paths).")
make_schema_key(ui_file_browser_schema, "file-browser-selected-bookmark", "s", "", "File browser selected bookmark", "Selected bookmark in file browser.")
make_schema_key(ui_file_browser_schema, "autoplay", "b", "true", "Autoplay", "Whether to autoplay the selected file.")
make_schema_key(ui_file_browser_schema, "show-unsupported-files", "b", "false", "Show unsupported files", "Whether to show unsupported files in the file browser.")
make_schema_key(ui_file_browser_schema, "show-hidden-files", "b", "false", "Show hidden files", "Whether to show hidden files in the file browser.")
make_schema_key_with_enum(ui_file_browser_schema, "filter", "file-browser-filter", "none", "File browser filter", "Selected file browser filter")
make_schema_key(ui_file_browser_schema, "last-location", "s", "", "Last location", "Last visited location.")
make_schema_key(ui_file_browser_schema, "instrument", "s", "", "Instrument", "Instrument plugin to use for MIDI auditioning.")

# UI - Mixer
ui_mixer_schema = make_schema(root, "ui.mixer")
make_schema_key(ui_mixer_schema, "inserts-expanded", "b", "true", "Inserts expanded", "Whether inserts are expanded.")
make_schema_key(ui_mixer_schema, "midi-fx-expanded", "b", "false", "MIDI FX expanded", "Whether MIDI FX are expanded.")
make_schema_key(ui_mixer_schema, "sends-expanded", "b", "false", "Sends expanded", "Whether sends are expanded.")

# UI - Inspector
ui_inspector_schema = make_schema(root, "ui.inspector")
make_schema_key(ui_inspector_schema, "track-properties-expanded", "b", "true", "Track properties expanded", "Whether track properties is expanded.")
make_schema_key(ui_inspector_schema, "track-outputs-expanded", "b", "true", "Track outputs expanded", "Whether track outputs is expanded.")
make_schema_key(ui_inspector_schema, "track-sends-expanded", "b", "true", "Track sends expanded", "Whether track sends is expanded.")
make_schema_key(ui_inspector_schema, "track-inputs-expanded", "b", "true", "Track inputs expanded", "Whether track inputs is expanded.")
make_schema_key(ui_inspector_schema, "track-controls-expanded", "b", "true", "Track controls expanded", "Whether track controls is expanded.")
make_schema_key(ui_inspector_schema, "track-inserts-expanded", "b", "true", "Track inserts expanded", "Whether track inserts is expanded.")
make_schema_key(ui_inspector_schema, "track-midi-fx-expanded", "b", "true", "Track midi-fx expanded", "Whether track midi-fx is expanded.")
make_schema_key(ui_inspector_schema, "track-fader-expanded", "b", "true", "Track fader expanded", "Whether track fader is expanded.")
make_schema_key(ui_inspector_schema, "track-comment-expanded", "b", "true", "Track comment expanded", "Whether track comment is expanded.")

# UI - Panels
ui_panels_schema = make_schema(root, "ui.panels")
make_schema_key(ui_panels_schema, "track-visibility-detached", "b", "false", "Track visibility detached", "Whether the track visibility panel is detached.")
make_schema_key(ui_panels_schema, "track-visibility-size", "(ii)", "(128,128)", "Track visibility size", "Track visibility panel size when detached.")
make_schema_key(ui_panels_schema, "track-inspector-detached", "b", "false", "Track inspector detached", "Whether the track inspector panel is detached.")
make_schema_key(ui_panels_schema, "track-inspector-size", "(ii)", "(128,128)", "Track inspector size", "Track inspector panel size when detached.")
make_schema_key(ui_panels_schema, "plugin-inspector-detached", "b", "false", "Plugin inspector detached", "Whether the plugin inspector panel is detached.")
make_schema_key(ui_panels_schema, "plugin-inspector-size", "(ii)", "(128,128)", "Plugin inspector size", "Plugin inspector panel size when detached.")
make_schema_key(ui_panels_schema, "plugin-browser-detached", "b", "false", "Plugin browser detached", "Whether the plugin browser panel is detached.")
make_schema_key(ui_panels_schema, "plugin-browser-size", "(ii)", "(128,128)", "Plugin browser size", "Plugin browser panel size when detached.")
make_schema_key(ui_panels_schema, "file-browser-detached", "b", "false", "File browser detached", "Whether the file browser panel is detached.")
make_schema_key(ui_panels_schema, "chord-pack-browser-detached", "b", "false", "Chord pack browser detached", "Whether the chord pack browser panel is detached.")
make_schema_key(ui_panels_schema, "file-browser-size", "(ii)", "(128,128)", "File browser size", "File browser panel size when detached.")
make_schema_key(ui_panels_schema, "monitor-section-detached", "b", "false", "Monitor section detached", "Whether the monitor section panel is detached.")
make_schema_key(ui_panels_schema, "monitor-section-size", "(ii)", "(128,128)", "Monitor section size", "Monitor section panel size when detached.")
make_schema_key(ui_panels_schema, "modulator-view-detached", "b", "false", "Modulator view detached", "Whether the modulator view panel is detached.")
make_schema_key(ui_panels_schema, "modulator-view-size", "(ii)", "(128,128)", "Modulator view size", "Modulator view panel size when detached.")
make_schema_key(ui_panels_schema, "mixer-detached", "b", "false", "Mixer detached", "Whether the mixer panel is detached.")
make_schema_key(ui_panels_schema, "mixer-size", "(ii)", "(128,128)", "Mixer size", "Mixer panel size when detached.")
make_schema_key(ui_panels_schema, "clip-editor-detached", "b", "false", "Clip editor detached", "Whether the clip editor panel is detached.")
make_schema_key(ui_panels_schema, "clip-editor-size", "(ii)", "(128,128)", "Clip editor size", "Clip editor panel size when detached.")
make_schema_key(ui_panels_schema, "chord-pad-detached", "b", "false", "Chord pad detached", "Whether the chord pad panel is detached.")
make_schema_key(ui_panels_schema, "chord-pad-size", "(ii)", "(128,128)", "Chord pad size", "Chord pad panel size when detached.")
make_schema_key(ui_panels_schema, "timeline-detached", "b", "false", "Timeline detached", "Whether the timeline panel is detached.")
make_schema_key(ui_panels_schema, "timeline-size", "(ii)", "(128,128)", "Timeline size", "Timeline panel size when detached.")
make_schema_key(ui_panels_schema, "cc-bindings-detached", "b", "false", "CC bindings detached", "Whether the CC bindings panel is detached.")
make_schema_key(ui_panels_schema, "cc-bindings-size", "(ii)", "(128,128)", "CC bindings size", "CC bindings panel size when detached.")
make_schema_key(ui_panels_schema, "port-connections-detached", "b", "false", "Port connections detached", "Whether the port connections panel is detached.")
make_schema_key(ui_panels_schema, "port-connections-size", "(ii)", "(128,128)", "Port connections size", "Port connections panel size when detached.")
make_schema_key(ui_panels_schema, "scenes-detached", "b", "false", "Scenes detached", "Whether the scenes panel is detached.")
make_schema_key(ui_panels_schema, "scenes-size", "(ii)", "(128,128)", "Scenes size", "Scenes panel size when detached.")

# Export - Audio
export_audio_schema = make_schema(root, "export.audio")
make_schema_key(export_audio_schema, "genre", "s", "Electronic", "Genre", "Genre to use when exporting, if the file type supports it.")
make_schema_key(export_audio_schema, "artist", "s", "Zrythm", "Artist", "Artist to use when exporting, if the file type supports it.")
make_schema_key(export_audio_schema, "title", "s", "My Project", "Title", "Title to use when exporting, if the file type supports it.")
make_schema_key_with_enum(export_audio_schema, "time-range", "export-time-range", "song", "Time range", "Time range to export.")
make_schema_key(export_audio_schema, "format", "s", "FLAC", "Format", "Format to export to.")
make_schema_key_with_enum(export_audio_schema, "filename-pattern", "export-filename-pattern", "append-format", "Filename pattern", "Filename pattern for exported files.")
make_schema_key(export_audio_schema, "dither", "b", "false", "Dither", "Add low level noise to reduce errors on lower bit depths.")
make_schema_key(export_audio_schema, "export-stems", "b", "false", "Export stems", "Whether to export stems instead of the mixdown.")
make_schema_key(export_audio_schema, "bit-depth", "i", "24", "Bit depth", "Bit depth to use when exporting")

# Export - MIDI
export_midi_schema = make_schema(root, "export.midi")
make_schema_key(export_midi_schema, "genre", "s", "Electronic", "Genre", "Genre to use when exporting.")
make_schema_key(export_midi_schema, "artist", "s", "Zrythm", "Artist", "Artist to use when exporting.")
make_schema_key(export_midi_schema, "title", "s", "My Project", "Title", "Title to use when exporting.")
make_schema_key_with_enum(export_midi_schema, "time-range", "export-time-range", "song", "Time range", "Time range to export.")
make_schema_key_with_range(export_midi_schema, "format", "i", "0", "1", "1", "Format", "MIDI file format (Type 0 or 1).")
make_schema_key_with_enum(export_midi_schema, "filename-pattern", "export-filename-pattern", "append-format", "Filename pattern", "Filename pattern for exported files.")
make_schema_key(export_midi_schema, "lanes-as-tracks", "b", "false", "Lanes as tracks", "Export track lanes as separate MIDI tracks.")
make_schema_key(export_midi_schema, "export-stems", "b", "false", "Export stems", "Whether to export stems instead of the mixdown.")

# Transport
transport_schema = make_schema(root, "transport")
make_schema_key(transport_schema, "loop", "b", "true", "Transport loop", "Whether looping is enabled.")
make_schema_key(transport_schema, "return-to-cue", "b", "true", "Return to cue", "Whether return to cue on stop is enabled.")
make_schema_key(transport_schema, "metronome-enabled", "b", "false", "Metronome enabled", "Whether the metronome is enabled.")
make_schema_key_with_range(transport_schema, "metronome-volume", "d", "0.0", "2.0", "1.0", "Metronome volume", "The metronome volume in amplitude (0 to 2).")
make_schema_key_with_enum(transport_schema, "metronome-countin", "preroll-count", "none", "Metronome Count-in", "Count-in bars for the metronome.")
make_schema_key(transport_schema, "punch-mode", "b", "false", "Punch mode enabled", "Whether punch in/out is enabled for recording.")
make_schema_key(transport_schema, "start-on-midi-input", "b", "false", "Start playback on MIDI input", "Whether to start playback on MIDI input.")
make_schema_key_with_enum(transport_schema, "recording-mode", "recording-mode", "takes", "Recording mode", "Recording mode.")
make_schema_key_with_enum(transport_schema, "recording-preroll", "preroll-count", "none", "Recording Pre-Roll", "Number of bars to pre-roll when recording.")

# === Preferences ===

# -- print preferences schemas --
# the first key in each schema should be called "info" and have the group in
# the summary and the subgroup in the description the value is:
#   [ sort index of group, sort index of child ]

# General

pref_general_engine_schema = make_preferences_schema("general.engine")
make_schema_key(pref_general_engine_schema, "info", "ai", "[0,0]", "General", "Engine")
make_schema_key_with_enum(pref_general_engine_schema, "audio-backend", "audio-backend", "none", "Audio backend", "")
make_schema_key(pref_general_engine_schema, "rtaudio-audio-device-name", "s", "", "RtAudio device", "")
make_schema_key(pref_general_engine_schema, "sdl-audio-device-name", "s", "", "SDL device", "")
make_schema_key_with_enum(pref_general_engine_schema, "sample-rate", "sample-rate", "48000", "Samplerate", "Samplerate to pass to the backend.")
make_schema_key_with_enum(pref_general_engine_schema, "buffer-size", "buffer-size", "512", "Buffer size", "Buffer size to pass to the backend.")
make_schema_key_with_enum(pref_general_engine_schema, "midi-backend", "midi-backend", "none", "MIDI backend", "")
make_schema_key(pref_general_engine_schema, "audio-inputs", "as", "[]", "Audio inputs", "A list of audio inputs to enable.")
make_schema_key(pref_general_engine_schema, "midi-controllers", "as", "[]", "MIDI controllers", "A list of controllers to enable.")
    
pref_general_paths_schema = make_preferences_schema("general.paths")
make_schema_key(pref_general_paths_schema, "info", "ai", "[0,1]", "General", "Paths")
make_schema_key(pref_general_paths_schema, "zrythm-dir", "s", "", "Zrythm path", "Directory used to save user data in.")

pref_general_updates_schema = make_preferences_schema("general.updates")
make_schema_key(pref_general_updates_schema, "info", "ai", "[0,2]", "General", "Updates")
make_schema_key(pref_general_updates_schema, "check-for-updates", "b", "false", "Check for updates", "Whether to check for updates on startup.")

pref_general_other_schema = make_preferences_schema("general.other")
make_schema_key(pref_general_other_schema, "info", "ai", "[0,3]", "General", "Other")

# Plugins

pref_plugins_uis_schema = make_preferences_schema("plugins.uis")
make_schema_key(pref_plugins_uis_schema, "info", "ai", "[1,0]", "Plugins", "UIs")
make_schema_key(pref_plugins_uis_schema, "open-on-instantiate", "b", "true", "Open UI on instantiation", "Open plugin UIs when they are instantiated.")
make_schema_key(pref_plugins_uis_schema, "stay-on-top", "b", "true", "Keep window on top", "Show plugin UIs on top of the main window (if possible).")
make_schema_key_with_range(pref_plugins_uis_schema, "refresh-rate", "i", "0", "180", "0", "Refresh rate", "Refresh rate in Hz. If set to 0, the screen's refresh rate will be used.")
make_schema_key_with_range(pref_plugins_uis_schema, "scale-factor", "d", "0.0", "4.0", "0.0", "Scale factor", "Scale factor to pass to plugin UIs. If set to 0, the monitor's scale factor will be used.")

pref_plugins_paths_schema = make_preferences_schema("plugins.paths")
make_schema_key(pref_plugins_paths_schema, "info", "ai", "[1,1]", "Plugins", "Paths")
make_schema_key(pref_plugins_paths_schema, "vst2-search-paths", "as", "[]", "VST2 plugins", "VST2 plugin search paths.")
make_schema_key(pref_plugins_paths_schema, "vst3-search-paths", "as", "[]", "VST3 plugins", "VST3 plugin search paths.")
make_schema_key(pref_plugins_paths_schema, "clap-search-paths", "as", "[]", "CLAP plugins", "CLAP plugin search paths.")
make_schema_key(pref_plugins_paths_schema, "lv2-search-paths", "as", "[]", "LV2 plugins", "LV2 plugin search paths. Paths for plugins bundled with Zrythm will be appended to this list.")
make_schema_key(pref_plugins_paths_schema, "dssi-search-paths", "as", "[]", "DSSI plugins", "DSSI plugin search paths.")
make_schema_key(pref_plugins_paths_schema, "ladspa-search-paths", "as", "[]", "LADSPA plugins", "LADSPA plugin search paths.")
make_schema_key(pref_plugins_paths_schema, "jsfx-search-paths", "as", "[]", "JSFX plugins", "JSFX plugin search paths.")
make_schema_key(pref_plugins_paths_schema, "sfz-search-paths", "as", "[]", "SFZ instruments", "SFZ instrument search paths.")
make_schema_key(pref_plugins_paths_schema, "sf2-search-paths", "as", "[]", "SF2 instruments", "SF2 instrument search paths.")

# Editing 

pref_editing_audio_schema = make_preferences_schema("editing.audio")
make_schema_key(pref_editing_audio_schema, "info", "ai", "[3,0]", "Editing", "Audio")
make_schema_key_with_enum(pref_editing_audio_schema, "fade-algorithm", "curve-algorithm", "superellipse", "Fade algorithm", "Default fade in/out algorithm.")

pref_editing_automation_schema = make_preferences_schema("editing.automation")
make_schema_key(pref_editing_automation_schema, "info", "ai", "[3,1]", "Editing", "Automation")
make_schema_key_with_enum(pref_editing_automation_schema, "curve-algorithm", "curve-algorithm", "superellipse", "Curve algorithm", "Default automation curve algorithm.")

pref_editing_undo_schema = make_preferences_schema("editing.undo")
make_schema_key(pref_editing_undo_schema, "info", "ai", "[3,2]", "Editing", "Undo")
make_schema_key_with_range(pref_editing_undo_schema, "undo-stack-length", "i", "-1", "380000", "128", "Undo stack length", "Maximum undo history stack length. Set to -1 for unlimited.")

# Projects

pref_projects_general_schema = make_preferences_schema("projects.general")
make_schema_key(pref_projects_general_schema, "info", "ai", "[4,0]", "Projects", "General")
make_schema_key_with_range(pref_projects_general_schema, "autosave-interval", "u", "0", "120", "1", "Autosave interval", "Interval to auto-save project backups, in minutes. Set to 0 to disable.")

# UI

pref_ui_general_schema = make_preferences_schema("ui.general")
make_schema_key(pref_ui_general_schema, "info", "ai", "[5,0]", "UI", "General")
make_schema_key_with_enum(pref_ui_general_schema, "graphic-detail", "graphic-detail", "high", "Draw detail", "Level of detail to use when drawing graphics.")
make_schema_key_with_range(pref_ui_general_schema, "font-scale", "d", "0.2", "2.0", "1.0", "Font scale", "Font scaling.")
make_schema_key_with_enum(pref_ui_general_schema, "language", "language", "en", "User interface language", "")
make_schema_key(pref_ui_general_schema, "css-theme", "s", "zrythm-theme.css", "CSS theme", "")
make_schema_key(pref_ui_general_schema, "icon-theme", "s", "zrythm-dark", "Icon theme", "")

# DSP

pref_dsp_pan_schema = make_preferences_schema("dsp.pan")
make_schema_key(pref_dsp_pan_schema, "info", "ai", "[2,0]", "DSP", "Pan")
make_schema_key_with_enum(pref_dsp_pan_schema, "pan-algorithm", "pan-algorithm", "sine", "Pan algorithm", "Not used at the moment.")
make_schema_key_with_enum(pref_dsp_pan_schema, "pan-law", "pan-law", "minus-three-db", "Pan law", "Not used at the moment.")

# === Export ===

tree = ET.ElementTree(root)

print ("Writing to " + output_file)
tree.write(output_file, encoding="utf-8", xml_declaration=True)

# === Pretty-print ===

print ("Pretty-printing XML...")
with open(output_file, "r", encoding="utf-8") as file:
    xml_string = file.read()

dom = xml.dom.minidom.parseString(xml_string)
pretty_xml = dom.toprettyxml(indent="  ", encoding="utf-8").decode("utf-8")

print ("Writing pretty-printed XML to " + output_file)
with open(output_file, "w", encoding="utf-8") as file:
    file.write(pretty_xml)