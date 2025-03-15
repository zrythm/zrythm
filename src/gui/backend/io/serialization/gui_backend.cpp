// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/audio_clip_editor.h"
#include "gui/backend/backend/automation_editor.h"
#include "gui/backend/backend/chord_editor.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/timeline.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/quantize_options.h"
#include "gui/dsp/snap_grid.h"

void
EditorSettings::define_base_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("scrollStartX", scroll_start_x_),
    make_field ("scrollStartY", scroll_start_y_),
    make_field ("horizontalZoomLevel", hzoom_level_));
}

void
PianoRoll::define_fields (const Context &ctx)
{
  using T = ISerializable<PianoRoll>;
  T::call_all_base_define_fields<EditorSettings> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("notesZoom", notes_zoom_),
    T::make_field ("midiModifier", midi_modifier_));
}

void
AutomationEditor::define_fields (const Context &ctx)
{
  using T = ISerializable<AutomationEditor>;
  T::call_all_base_define_fields<EditorSettings> (ctx);
}

void
ChordEditor::define_fields (const Context &ctx)
{
  using T = ISerializable<ChordEditor>;
  T::call_all_base_define_fields<EditorSettings> (ctx);
  T::serialize_fields (ctx, T::make_field ("chords", chords_));
}

void
AudioClipEditor::define_fields (const Context &ctx)
{
  using T = ISerializable<AudioClipEditor>;
  T::call_all_base_define_fields<EditorSettings> (ctx);
}

void
Timeline::define_fields (const Context &ctx)
{
  using T = ISerializable<Timeline>;
  T::call_all_base_define_fields<EditorSettings> (ctx);
  T::serialize_fields (
    ctx,
    // available after 1.8
    T::make_field ("tracksWidth", tracks_width_, true));
}

void
ClipEditor::define_fields (const Context &ctx)
{
  using T = ISerializable<ClipEditor>;
  T::serialize_fields (
    ctx, T::make_field ("regionId", region_id_),
    T::make_field ("pianoRoll", piano_roll_),
    T::make_field ("automationEditor", automation_editor_),
    T::make_field ("chordEditor", chord_editor_),
    T::make_field ("audioClipEditor", audio_clip_editor_));
}

void
zrythm::gui::SnapGrid::define_fields (const Context &ctx)
{
  using T = ISerializable<SnapGrid>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_),
    T::make_field ("snapNoteLength", snap_note_length_),
    T::make_field ("snapNoteType", snap_note_type_),
    T::make_field ("snapAdaptive", snap_adaptive_),
    T::make_field ("defaultNoteLength", default_note_length_),
    T::make_field ("defaultNoteType", default_note_type_),
    T::make_field ("defaultAdaptive", default_adaptive_),
    T::make_field ("lengthType", length_type_),
    T::make_field ("snapToGrid", snap_to_grid_),
    T::make_field ("keepOffset", snap_to_grid_keep_offset_),
    T::make_field ("snapToEvents", snap_to_events_));
}

void
zrythm::gui::old_dsp::QuantizeOptions::define_fields (const Context &ctx)
{
  using T = ISerializable<QuantizeOptions>;
  T::serialize_fields (
    ctx, T::make_field ("noteLength", note_length_),
    T::make_field ("noteType", note_type_), T::make_field ("amount", amount_),
    T::make_field ("adjStart", adj_start_), T::make_field ("adjEnd", adj_end_),
    T::make_field ("swing", swing_), T::make_field ("randTicks", rand_ticks_));
}
