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
  yyjson_mut_obj_add_bool (doc, sel_obj, "poolId", sel->pool_id);
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
  yyjson_mut_val * tracks_arr = yyjson_mut_obj_add_arr (doc, sel_obj, "tracks");
  for (int i = 0; i < sel->num_tracks; i++)
    {
      Track * track = sel->tracks[i];
      /* only save the index when serializing project tracklist selections */
      if (sel->is_project)
        {
          yyjson_mut_arr_add_int (doc, tracks_arr, track->pos);
        }
      else
        {
          yyjson_mut_val * track_obj = yyjson_mut_arr_add_obj (doc, tracks_arr);
          track_serialize_to_json (doc, track_obj, track, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, sel_obj, "isProject", sel->is_project);
  return true;
}
