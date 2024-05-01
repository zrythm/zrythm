// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/audio_selections.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/extra.h"
#include "io/serialization/plugin.h"
#include "io/serialization/selections.h"
#include "io/serialization/track.h"
#include "utils/objects.h"

typedef enum
{
  Z_IO_SERIALIZATION_SELECTIONS_ERROR_FAILED,
} ZIOSerializationSelectionsError;

#define Z_IO_SERIALIZATION_SELECTIONS_ERROR \
  z_io_serialization_selections_error_quark ()
GQuark
z_io_serialization_selections_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - selections - error - quark, z_io_serialization_selections_error)

bool
mixer_selections_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        sel_obj,
  const MixerSelections * sel,
  GError **               error)
{
  yyjson_mut_obj_add_int (doc, sel_obj, "type", sel->type);
  yyjson_mut_val * slots_arr = yyjson_mut_obj_add_arr (doc, sel_obj, "slots");
  for (int i = 0; i < sel->num_slots; i++)
    {
      yyjson_mut_arr_add_int (doc, slots_arr, sel->slots[i]);
    }
  yyjson_mut_val * plugins_arr =
    yyjson_mut_obj_add_arr (doc, sel_obj, "plugins");
  for (int i = 0; i < sel->num_slots; i++)
    {
      yyjson_mut_val * plugin_obj = yyjson_mut_arr_add_obj (doc, plugins_arr);
      plugin_serialize_to_json (doc, plugin_obj, sel->plugins[i], error);
    }
  yyjson_mut_obj_add_uint (doc, sel_obj, "trackNameHash", sel->track_name_hash);
  yyjson_mut_obj_add_bool (doc, sel_obj, "hasAny", sel->has_any);
  return true;
}

bool
arranger_selections_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           sel_obj,
  const ArrangerSelections * sel,
  GError **                  error)
{
  yyjson_mut_obj_add_int (doc, sel_obj, "type", sel->type);
  return true;
}

bool
timeline_selections_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           sel_obj,
  const TimelineSelections * sel,
  GError **                  error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, sel_obj, "base");
  arranger_selections_serialize_to_json (doc, base_obj, &sel->base, error);
  if (sel->regions)
    {
      yyjson_mut_val * regions_arr =
        yyjson_mut_obj_add_arr (doc, sel_obj, "regions");
      for (int i = 0; i < sel->num_regions; i++)
        {
          ZRegion *        r = sel->regions[i];
          yyjson_mut_val * r_obj = yyjson_mut_arr_add_obj (doc, regions_arr);
          region_serialize_to_json (doc, r_obj, r, error);
        }
    }
  if (sel->scale_objects)
    {
      yyjson_mut_val * scale_objects_arr =
        yyjson_mut_obj_add_arr (doc, sel_obj, "scaleObjects");
      for (int i = 0; i < sel->num_scale_objects; i++)
        {
          ScaleObject *    r = sel->scale_objects[i];
          yyjson_mut_val * r_obj =
            yyjson_mut_arr_add_obj (doc, scale_objects_arr);
          scale_object_serialize_to_json (doc, r_obj, r, error);
        }
    }
  if (sel->markers)
    {
      yyjson_mut_val * markers_arr =
        yyjson_mut_obj_add_arr (doc, sel_obj, "markers");
      for (int i = 0; i < sel->num_markers; i++)
        {
          Marker *         r = sel->markers[i];
          yyjson_mut_val * r_obj = yyjson_mut_arr_add_obj (doc, markers_arr);
          marker_serialize_to_json (doc, r_obj, r, error);
        }
    }
  yyjson_mut_obj_add_int (
    doc, sel_obj, "regionTrackVisibilityIndex", sel->region_track_vis_index);
  yyjson_mut_obj_add_int (
    doc, sel_obj, "chordTrackVisibilityIndex", sel->chord_track_vis_index);
  yyjson_mut_obj_add_int (
    doc, sel_obj, "markerTrackVisibilityIndex", sel->marker_track_vis_index);
  return true;
}

bool
midi_arranger_selections_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               sel_obj,
  const MidiArrangerSelections * sel,
  GError **                      error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, sel_obj, "base");
  arranger_selections_serialize_to_json (doc, base_obj, &sel->base, error);
  if (sel->midi_notes)
    {
      yyjson_mut_val * midi_notes_arr =
        yyjson_mut_obj_add_arr (doc, sel_obj, "midiNotes");
      for (int i = 0; i < sel->num_midi_notes; i++)
        {
          MidiNote *       r = sel->midi_notes[i];
          yyjson_mut_val * r_obj = yyjson_mut_arr_add_obj (doc, midi_notes_arr);
          midi_note_serialize_to_json (doc, r_obj, r, error);
        }
    }
  return true;
}

bool
chord_selections_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        sel_obj,
  const ChordSelections * sel,
  GError **               error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, sel_obj, "base");
  arranger_selections_serialize_to_json (doc, base_obj, &sel->base, error);
  if (sel->chord_objects)
    {
      yyjson_mut_val * chord_objects_arr =
        yyjson_mut_obj_add_arr (doc, sel_obj, "chordObjects");
      for (int i = 0; i < sel->num_chord_objects; i++)
        {
          ChordObject *    r = sel->chord_objects[i];
          yyjson_mut_val * r_obj =
            yyjson_mut_arr_add_obj (doc, chord_objects_arr);
          chord_object_serialize_to_json (doc, r_obj, r, error);
        }
    }
  return true;
}

bool
automation_selections_serialize_to_json (
  yyjson_mut_doc *             doc,
  yyjson_mut_val *             sel_obj,
  const AutomationSelections * sel,
  GError **                    error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, sel_obj, "base");
  arranger_selections_serialize_to_json (doc, base_obj, &sel->base, error);
  if (sel->automation_points)
    {
      yyjson_mut_val * automation_points_arr =
        yyjson_mut_obj_add_arr (doc, sel_obj, "automationPoints");
      for (int i = 0; i < sel->num_automation_points; i++)
        {
          AutomationPoint * r = sel->automation_points[i];
          yyjson_mut_val *  r_obj =
            yyjson_mut_arr_add_obj (doc, automation_points_arr);
          automation_point_serialize_to_json (doc, r_obj, r, error);
        }
    }
  return true;
}

bool
audio_selections_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        sel_obj,
  const AudioSelections * sel,
  GError **               error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, sel_obj, "base");
  arranger_selections_serialize_to_json (doc, base_obj, &sel->base, error);
  yyjson_mut_obj_add_bool (doc, sel_obj, "hasSelection", sel->has_selection);
  yyjson_mut_val * sel_start_obj =
    yyjson_mut_obj_add_obj (doc, sel_obj, "selStart");
  position_serialize_to_json (doc, sel_start_obj, &sel->sel_start, error);
  yyjson_mut_val * sel_end_obj = yyjson_mut_obj_add_obj (doc, sel_obj, "selEnd");
  position_serialize_to_json (doc, sel_end_obj, &sel->sel_end, error);
  yyjson_mut_obj_add_int (doc, sel_obj, "poolId", sel->pool_id);
  yyjson_mut_val * region_id_obj =
    yyjson_mut_obj_add_obj (doc, sel_obj, "regionId");
  region_identifier_serialize_to_json (
    doc, region_id_obj, &sel->region_id, error);
  return true;
}

bool
tracklist_selections_serialize_to_json (
  yyjson_mut_doc *            doc,
  yyjson_mut_val *            sel_obj,
  const TracklistSelections * sel,
  GError **                   error)
{
  yyjson_mut_obj_add_bool (doc, sel_obj, "isProject", sel->is_project);
  yyjson_mut_val * tracks_arr = yyjson_mut_obj_add_arr (doc, sel_obj, "tracks");
  for (int i = 0; i < sel->num_tracks; i++)
    {
      Track *          track = sel->tracks[i];
      yyjson_mut_val * track_obj = yyjson_mut_arr_add_obj (doc, tracks_arr);
      track_serialize_to_json (doc, track_obj, track, error);
    }
  return true;
}

bool
mixer_selections_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      sel_obj,
  MixerSelections * sel,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  sel->type =
    (ZPluginSlotType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  yyjson_val *    slots_arr = yyjson_obj_iter_get (&it, "slots");
  yyjson_arr_iter slot_it = yyjson_arr_iter_with (slots_arr);
  yyjson_val *    slot_obj = NULL;
  while ((slot_obj = yyjson_arr_iter_next (&slot_it)))
    {
      sel->slots[sel->num_slots++] = yyjson_get_int (slot_obj);
    }
  yyjson_val *    plugins_arr = yyjson_obj_iter_get (&it, "plugins");
  yyjson_arr_iter plugin_it = yyjson_arr_iter_with (plugins_arr);
  yyjson_val *    plugin_obj = NULL;
  size_t          count = 0;
  while ((plugin_obj = yyjson_arr_iter_next (&plugin_it)))
    {
      Plugin * pl = object_new (Plugin);
      sel->plugins[count++] = pl;
      plugin_deserialize_from_json (doc, plugin_obj, pl, error);
    }
  sel->track_name_hash =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "trackNameHash"));
  sel->has_any = yyjson_get_bool (yyjson_obj_iter_get (&it, "hasAny"));
  return true;
}

bool
arranger_selections_deserialize_from_json (
  yyjson_doc *         doc,
  yyjson_val *         sel_obj,
  ArrangerSelections * sel,
  GError **            error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  sel->type =
    (ArrangerSelectionsType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  return true;
}

bool
timeline_selections_deserialize_from_json (
  yyjson_doc *         doc,
  yyjson_val *         sel_obj,
  TimelineSelections * sel,
  GError **            error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_selections_deserialize_from_json (doc, base_obj, &sel->base, error);
  yyjson_val * regions_arr = yyjson_obj_iter_get (&it, "regions");
  sel->regions_size = yyjson_arr_size (regions_arr);
  if (sel->regions_size > 0)
    {
      sel->regions = g_malloc0_n (sel->regions_size, sizeof (ZRegion *));
      yyjson_arr_iter region_it = yyjson_arr_iter_with (regions_arr);
      yyjson_val *    region_obj = NULL;
      while ((region_obj = yyjson_arr_iter_next (&region_it)))
        {
          ZRegion * r = object_new (ZRegion);
          sel->regions[sel->num_regions++] = r;
          region_deserialize_from_json (doc, region_obj, r, error);
        }
    }
  yyjson_val * scale_objects_arr = yyjson_obj_iter_get (&it, "scaleObjects");
  sel->scale_objects_size = yyjson_arr_size (scale_objects_arr);
  if (sel->scale_objects_size > 0)
    {
      sel->scale_objects =
        g_malloc0_n (sel->scale_objects_size, sizeof (ScaleObject *));
      yyjson_arr_iter scale_object_it = yyjson_arr_iter_with (scale_objects_arr);
      yyjson_val * scale_object_obj = NULL;
      while ((scale_object_obj = yyjson_arr_iter_next (&scale_object_it)))
        {
          ScaleObject * r = object_new (ScaleObject);
          sel->scale_objects[sel->num_scale_objects++] = r;
          scale_object_deserialize_from_json (doc, scale_object_obj, r, error);
        }
    }
  yyjson_val * markers_arr = yyjson_obj_iter_get (&it, "markers");
  sel->markers_size = yyjson_arr_size (markers_arr);
  if (sel->markers_size > 0)
    {
      sel->markers = g_malloc0_n (sel->markers_size, sizeof (Marker *));
      yyjson_arr_iter marker_it = yyjson_arr_iter_with (markers_arr);
      yyjson_val *    marker_obj = NULL;
      while ((marker_obj = yyjson_arr_iter_next (&marker_it)))
        {
          Marker * r = object_new (Marker);
          sel->markers[sel->num_markers++] = r;
          marker_deserialize_from_json (doc, marker_obj, r, error);
        }
    }
  sel->region_track_vis_index =
    yyjson_get_int (yyjson_obj_iter_get (&it, "regionTrackVisibilityIndex"));
  sel->chord_track_vis_index =
    yyjson_get_int (yyjson_obj_iter_get (&it, "chordTrackVisibilityIndex"));
  sel->marker_track_vis_index =
    yyjson_get_int (yyjson_obj_iter_get (&it, "markerTrackVisibilityIndex"));
  return true;
}

bool
midi_arranger_selections_deserialize_from_json (
  yyjson_doc *             doc,
  yyjson_val *             sel_obj,
  MidiArrangerSelections * sel,
  GError **                error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_selections_deserialize_from_json (doc, base_obj, &sel->base, error);
  yyjson_val * midi_notes_arr = yyjson_obj_iter_get (&it, "midiNotes");
  sel->midi_notes_size = yyjson_arr_size (midi_notes_arr);
  if (sel->midi_notes_size > 0)
    {
      sel->midi_notes = g_malloc0_n (sel->midi_notes_size, sizeof (MidiNote *));
      yyjson_arr_iter midi_note_it = yyjson_arr_iter_with (midi_notes_arr);
      yyjson_val *    midi_note_obj = NULL;
      while ((midi_note_obj = yyjson_arr_iter_next (&midi_note_it)))
        {
          MidiNote * r = object_new (MidiNote);
          sel->midi_notes[sel->num_midi_notes++] = r;
          midi_note_deserialize_from_json (doc, midi_note_obj, r, error);
        }
    }
  return true;
}

bool
chord_selections_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      sel_obj,
  ChordSelections * sel,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_selections_deserialize_from_json (doc, base_obj, &sel->base, error);
  yyjson_val * chord_objects_arr = yyjson_obj_iter_get (&it, "chordObjects");
  sel->chord_objects_size = yyjson_arr_size (chord_objects_arr);
  if (sel->chord_objects_size > 0)
    {
      sel->chord_objects =
        g_malloc0_n (sel->chord_objects_size, sizeof (ChordObject *));
      yyjson_arr_iter chord_object_it = yyjson_arr_iter_with (chord_objects_arr);
      yyjson_val * chord_object_obj = NULL;
      while ((chord_object_obj = yyjson_arr_iter_next (&chord_object_it)))
        {
          ChordObject * r = object_new (ChordObject);
          sel->chord_objects[sel->num_chord_objects++] = r;
          chord_object_deserialize_from_json (doc, chord_object_obj, r, error);
        }
    }
  return true;
}

bool
automation_selections_deserialize_from_json (
  yyjson_doc *           doc,
  yyjson_val *           sel_obj,
  AutomationSelections * sel,
  GError **              error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_selections_deserialize_from_json (doc, base_obj, &sel->base, error);
  yyjson_val * automation_points_arr =
    yyjson_obj_iter_get (&it, "automationPoints");
  sel->automation_points_size = yyjson_arr_size (automation_points_arr);
  if (sel->automation_points_size > 0)
    {
      sel->automation_points =
        g_malloc0_n (sel->automation_points_size, sizeof (AutomationPoint *));
      yyjson_arr_iter automation_point_it =
        yyjson_arr_iter_with (automation_points_arr);
      yyjson_val * automation_point_obj = NULL;
      while (
        (automation_point_obj = yyjson_arr_iter_next (&automation_point_it)))
        {
          AutomationPoint * r = object_new (AutomationPoint);
          sel->automation_points[sel->num_automation_points++] = r;
          automation_point_deserialize_from_json (
            doc, automation_point_obj, r, error);
        }
    }
  return true;
}

bool
audio_selections_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      sel_obj,
  AudioSelections * sel,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_selections_deserialize_from_json (doc, base_obj, &sel->base, error);
  sel->has_selection =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "hasSelection"));
  yyjson_val * sel_start_obj = yyjson_obj_iter_get (&it, "selStart");
  position_deserialize_from_json (doc, sel_start_obj, &sel->sel_start, error);
  yyjson_val * sel_end_obj = yyjson_obj_iter_get (&it, "selEnd");
  position_deserialize_from_json (doc, sel_end_obj, &sel->sel_end, error);
  sel->pool_id = yyjson_get_int (yyjson_obj_iter_get (&it, "poolId"));
  yyjson_val * region_id_obj = yyjson_obj_iter_get (&it, "regionId");
  region_identifier_deserialize_from_json (
    doc, region_id_obj, &sel->region_id, error);
  return true;
}

bool
tracklist_selections_deserialize_from_json (
  yyjson_doc *          doc,
  yyjson_val *          sel_obj,
  TracklistSelections * sel,
  GError **             error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sel_obj);
  sel->is_project = yyjson_get_bool (yyjson_obj_iter_get (&it, "isProject"));
  yyjson_val *    tracks_arr = yyjson_obj_iter_get (&it, "tracks");
  yyjson_arr_iter track_it = yyjson_arr_iter_with (tracks_arr);
  yyjson_val *    track_obj = NULL;
  while ((track_obj = yyjson_arr_iter_next (&track_it)))
    {
      Track * track = object_new (Track);
      sel->tracks[sel->num_tracks++] = track;
      track_deserialize_from_json (doc, track_obj, track, error);
    }
  return true;
}
