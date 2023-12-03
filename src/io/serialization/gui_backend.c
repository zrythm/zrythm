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
#include "io/serialization/extra.h"
#include "io/serialization/gui_backend.h"

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
    yyjson_mut_obj_add_obj (doc, ce_obj, "AutomationEditor");
  automation_editor_serialize_to_json (
    doc, automation_editor_obj, ce->automation_editor, error);
  yyjson_mut_val * chord_editor_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "ChordEditor");
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
