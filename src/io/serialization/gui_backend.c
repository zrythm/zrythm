// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/quantize_options.h"
#include "dsp/snap_grid.h"
#include "gui/backend/audio_clip_editor.h"
#include "gui/backend/automation_editor.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/timeline.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/chords.h"
#include "io/serialization/extra.h"
#include "io/serialization/gui_backend.h"
#include "utils/objects.h"

typedef enum
{
  Z_IO_SERIALIZATION_GUI_BACKEND_ERROR_FAILED,
} ZIOSerializationGuiBackendError;

#define Z_IO_SERIALIZATION_GUI_BACKEND_ERROR \
  z_io_serialization_gui_backend_error_quark ()
GQuark
z_io_serialization_gui_backend_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - gui - backend - error - quark, z_io_serialization_gui_backend_error)

static bool
editor_settings_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       settings_obj,
  const EditorSettings * settings,
  GError **              error)
{
  yyjson_mut_obj_add_int (
    doc, settings_obj, "scrollStartX", settings->scroll_start_x);
  yyjson_mut_obj_add_int (
    doc, settings_obj, "scrollStartY", settings->scroll_start_y);
  yyjson_mut_obj_add_real (
    doc, settings_obj, "horizontalZoomLevel", settings->hzoom_level);
  return true;
}

static bool
piano_roll_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  pr_obj,
  const PianoRoll * pr,
  GError **         error)
{
  yyjson_mut_obj_add_real (doc, pr_obj, "notesZoom", pr->notes_zoom);
  yyjson_mut_obj_add_int (doc, pr_obj, "midiModifier", pr->midi_modifier);
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, pr_obj, "editorSettings");
  editor_settings_serialize_to_json (
    doc, editor_settings_obj, &pr->editor_settings, error);
  return true;
}

static bool
automation_editor_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         ae_obj,
  const AutomationEditor * ae,
  GError **                error)
{
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, ae_obj, "editorSettings");
  editor_settings_serialize_to_json (
    doc, editor_settings_obj, &ae->editor_settings, error);
  return true;
}

static bool
chord_editor_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    ce_obj,
  const ChordEditor * ce,
  GError **           error)
{
  yyjson_mut_val * chords_arr = yyjson_mut_obj_add_arr (doc, ce_obj, "chords");
  for (int i = 0; i < ce->num_chords; i++)
    {
      const ChordDescriptor * chord = ce->chords[i];
      yyjson_mut_val * chord_obj = yyjson_mut_arr_add_obj (doc, chords_arr);
      chord_descriptor_serialize_to_json (doc, chord_obj, chord, error);
    }
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "editorSettings");
  editor_settings_serialize_to_json (
    doc, editor_settings_obj, &ce->editor_settings, error);
  return true;
}

static bool
audio_clip_editor_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        editor_obj,
  const AudioClipEditor * editor,
  GError **               error)
{
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, editor_obj, "editorSettings");
  editor_settings_serialize_to_json (
    doc, editor_settings_obj, &editor->editor_settings, error);
  return true;
}

bool
clip_editor_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   ce_obj,
  const ClipEditor * ce,
  GError **          error)
{
  yyjson_mut_val * region_id_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "regionId");
  region_identifier_serialize_to_json (
    doc, region_id_obj, &ce->region_id, error);
  yyjson_mut_obj_add_bool (doc, ce_obj, "hasRegion", ce->has_region);
  yyjson_mut_val * piano_roll_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "pianoRoll");
  piano_roll_serialize_to_json (doc, piano_roll_obj, ce->piano_roll, error);
  yyjson_mut_val * automation_editor_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "automationEditor");
  automation_editor_serialize_to_json (
    doc, automation_editor_obj, ce->automation_editor, error);
  yyjson_mut_val * chord_editor_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "chordEditor");
  chord_editor_serialize_to_json (
    doc, chord_editor_obj, ce->chord_editor, error);
  yyjson_mut_val * audio_clip_editor_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "audioClipEditor");
  audio_clip_editor_serialize_to_json (
    doc, audio_clip_editor_obj, ce->audio_clip_editor, error);
  return true;
}

bool
timeline_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * t_obj,
  const Timeline * t,
  GError **        error)
{
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, t_obj, "editorSettings");
  editor_settings_serialize_to_json (
    doc, editor_settings_obj, &t->editor_settings, error);
  yyjson_mut_obj_add_int (doc, t_obj, "tracksWidth", t->tracks_width);
  return true;
}

bool
snap_grid_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * sg_obj,
  const SnapGrid * sg,
  GError **        error)
{
  yyjson_mut_obj_add_int (doc, sg_obj, "type", sg->type);
  yyjson_mut_obj_add_int (doc, sg_obj, "snapNoteLength", sg->snap_note_length);
  yyjson_mut_obj_add_int (doc, sg_obj, "snapNoteType", sg->snap_note_type);
  yyjson_mut_obj_add_bool (doc, sg_obj, "snapAdaptive", sg->snap_adaptive);
  yyjson_mut_obj_add_int (
    doc, sg_obj, "defaultNoteLength", sg->default_note_length);
  yyjson_mut_obj_add_int (doc, sg_obj, "defaultNoteType", sg->default_note_type);
  yyjson_mut_obj_add_bool (doc, sg_obj, "defaultAdaptive", sg->default_adaptive);
  yyjson_mut_obj_add_int (doc, sg_obj, "lengthType", sg->length_type);
  yyjson_mut_obj_add_bool (doc, sg_obj, "snapToGrid", sg->snap_to_grid);
  yyjson_mut_obj_add_bool (
    doc, sg_obj, "keepOffset", sg->snap_to_grid_keep_offset);
  yyjson_mut_obj_add_bool (doc, sg_obj, "snapToEvents", sg->snap_to_events);
  return true;
}

bool
quantize_options_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        qo_obj,
  const QuantizeOptions * qo,
  GError **               error)
{
  yyjson_mut_obj_add_int (doc, qo_obj, "noteLength", qo->note_length);
  yyjson_mut_obj_add_int (doc, qo_obj, "noteType", qo->note_type);
  yyjson_mut_obj_add_real (doc, qo_obj, "amount", qo->amount);
  yyjson_mut_obj_add_int (doc, qo_obj, "adjStart", qo->adj_start);
  yyjson_mut_obj_add_int (doc, qo_obj, "adjEnd", qo->adj_end);
  yyjson_mut_obj_add_real (doc, qo_obj, "swing", qo->swing);
  yyjson_mut_obj_add_real (doc, qo_obj, "randTicks", qo->rand_ticks);
  return true;
}

static bool
editor_settings_deserialize_from_json (
  yyjson_doc *     doc,
  yyjson_val *     settings_obj,
  EditorSettings * settings,
  GError **        error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (settings_obj);
  settings->scroll_start_x =
    yyjson_get_int (yyjson_obj_iter_get (&it, "scrollStartX"));
  settings->scroll_start_y =
    yyjson_get_int (yyjson_obj_iter_get (&it, "scrollStartY"));
  settings->hzoom_level =
    yyjson_get_real (yyjson_obj_iter_get (&it, "horizontalZoomLevel"));
  return true;
}

static bool
piano_roll_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * pr_obj,
  PianoRoll *  pr,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pr_obj);
  pr->notes_zoom =
    (float) yyjson_get_real (yyjson_obj_iter_get (&it, "notesZoom"));
  pr->midi_modifier =
    (MidiModifier) yyjson_get_int (yyjson_obj_iter_get (&it, "midiModifier"));
  yyjson_val * editor_settings_obj = yyjson_obj_iter_get (&it, "editorSettings");
  editor_settings_deserialize_from_json (
    doc, editor_settings_obj, &pr->editor_settings, error);
  return true;
}

static bool
automation_editor_deserialize_from_json (
  yyjson_doc *       doc,
  yyjson_val *       ae_obj,
  AutomationEditor * ae,
  GError **          error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (ae_obj);
  yyjson_val * editor_settings_obj = yyjson_obj_iter_get (&it, "editorSettings");
  editor_settings_deserialize_from_json (
    doc, editor_settings_obj, &ae->editor_settings, error);
  return true;
}

static bool
chord_editor_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  ce_obj,
  ChordEditor * ce,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (ce_obj);
  yyjson_val *    chords_arr = yyjson_obj_iter_get (&it, "chords");
  yyjson_arr_iter chords_it = yyjson_arr_iter_with (chords_arr);
  yyjson_val *    chord_obj = NULL;
  while ((chord_obj = yyjson_arr_iter_next (&chords_it)))
    {
      ChordDescriptor * descr = object_new (ChordDescriptor);
      ce->chords[ce->num_chords++] = descr;
      chord_descriptor_deserialize_from_json (doc, chord_obj, descr, error);
    }
  yyjson_val * editor_settings_obj = yyjson_obj_iter_get (&it, "editorSettings");
  editor_settings_deserialize_from_json (
    doc, editor_settings_obj, &ce->editor_settings, error);
  return true;
}

static bool
audio_clip_editor_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      editor_obj,
  AudioClipEditor * editor,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (editor_obj);
  yyjson_val * editor_settings_obj = yyjson_obj_iter_get (&it, "editorSettings");
  editor_settings_deserialize_from_json (
    doc, editor_settings_obj, &editor->editor_settings, error);
  return true;
}

bool
clip_editor_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * ce_obj,
  ClipEditor * ce,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (ce_obj);
  yyjson_val *    region_id_obj = yyjson_obj_iter_get (&it, "regionId");
  region_identifier_deserialize_from_json (
    doc, region_id_obj, &ce->region_id, error);
  ce->has_region = yyjson_get_bool (yyjson_obj_iter_get (&it, "hasRegion"));
  yyjson_val * piano_roll_obj = yyjson_obj_iter_get (&it, "pianoRoll");
  ce->piano_roll = object_new (PianoRoll);
  piano_roll_deserialize_from_json (doc, piano_roll_obj, ce->piano_roll, error);
  yyjson_val * automation_editor_obj =
    yyjson_obj_iter_get (&it, "automationEditor");
  ce->automation_editor = object_new (AutomationEditor);
  automation_editor_deserialize_from_json (
    doc, automation_editor_obj, ce->automation_editor, error);
  yyjson_val * chord_editor_obj = yyjson_obj_iter_get (&it, "chordEditor");
  ce->chord_editor = object_new (ChordEditor);
  chord_editor_deserialize_from_json (
    doc, chord_editor_obj, ce->chord_editor, error);
  yyjson_val * audio_clip_editor_obj =
    yyjson_obj_iter_get (&it, "audioClipEditor");
  ce->audio_clip_editor = object_new (AudioClipEditor);
  audio_clip_editor_deserialize_from_json (
    doc, audio_clip_editor_obj, ce->audio_clip_editor, error);
  return true;
}

bool
timeline_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * t_obj,
  Timeline *   t,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (t_obj);
  yyjson_val * editor_settings_obj = yyjson_obj_iter_get (&it, "editorSettings");
  editor_settings_deserialize_from_json (
    doc, editor_settings_obj, &t->editor_settings, error);
  yyjson_val * tracks_width_obj = yyjson_obj_iter_get (&it, "tracksWidth");
  if (tracks_width_obj) /* available after 1.8 */
    {
      t->tracks_width = yyjson_get_int (tracks_width_obj);
    }
  return true;
}

bool
snap_grid_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * sg_obj,
  SnapGrid *   sg,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sg_obj);
  sg->type = (SnapGridType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  sg->snap_note_length =
    (NoteLength) yyjson_get_int (yyjson_obj_iter_get (&it, "snapNoteLength"));
  sg->snap_note_type =
    (NoteType) yyjson_get_int (yyjson_obj_iter_get (&it, "snapNoteType"));
  sg->snap_adaptive =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "snapAdaptive"));
  sg->default_note_length = (NoteLength) yyjson_get_int (
    yyjson_obj_iter_get (&it, "defaultNoteLength"));
  sg->default_note_type =
    (NoteType) yyjson_get_int (yyjson_obj_iter_get (&it, "defaultNoteType"));
  sg->default_adaptive =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "defaultAdaptive"));
  sg->length_type =
    (NoteLengthType) yyjson_get_int (yyjson_obj_iter_get (&it, "lengthType"));
  sg->snap_to_grid = yyjson_get_bool (yyjson_obj_iter_get (&it, "snapToGrid"));
  sg->snap_to_grid_keep_offset =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "keepOffset"));
  sg->snap_to_events =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "snapToEvents"));
  return true;
}

bool
quantize_options_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      qo_obj,
  QuantizeOptions * qo,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (qo_obj);
  qo->note_length =
    (NoteLength) yyjson_get_int (yyjson_obj_iter_get (&it, "noteLength"));
  qo->note_type =
    (NoteType) yyjson_get_int (yyjson_obj_iter_get (&it, "noteType"));
  qo->amount = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "amount"));
  qo->adj_start = yyjson_get_int (yyjson_obj_iter_get (&it, "adjStart"));
  qo->adj_end = yyjson_get_int (yyjson_obj_iter_get (&it, "adjEnd"));
  qo->swing = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "swing"));
  qo->rand_ticks = yyjson_get_real (yyjson_obj_iter_get (&it, "randTicks"));
  return true;
}
