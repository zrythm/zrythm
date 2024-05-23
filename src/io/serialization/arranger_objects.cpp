// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/marker.h"
#include "dsp/region.h"
#include "dsp/scale_object.h"
#include "gui/backend/arranger_object.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/extra.h"
#include "utils/objects.h"

typedef enum
{
  Z_IO_SERIALIZATION_ARRANGER_OBJECTS_ERROR_FAILED,
} ZIOSerializationArrangerObjectsError;

#define Z_IO_SERIALIZATION_ARRANGER_OBJECTS_ERROR \
  z_io_serialization_arranger_objects_error_quark ()
GQuark
z_io_serialization_arranger_objects_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - arranger - objects - error - quark, z_io_serialization_arranger_objects_error)

bool
arranger_object_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       ao_obj,
  const ArrangerObject * ao,
  GError **              error)
{
  yyjson_mut_obj_add_int (doc, ao_obj, "type", (int64_t) ao->type);
  yyjson_mut_obj_add_int (doc, ao_obj, "flags", (int64_t) ao->flags);
  yyjson_mut_obj_add_bool (doc, ao_obj, "muted", ao->muted);
  yyjson_mut_val * pos_obj = yyjson_mut_obj_add_obj (doc, ao_obj, "pos");
  position_serialize_to_json (doc, pos_obj, &ao->pos, error);
  yyjson_mut_val * end_pos_obj = yyjson_mut_obj_add_obj (doc, ao_obj, "endPos");
  position_serialize_to_json (doc, end_pos_obj, &ao->end_pos, error);
  yyjson_mut_val * clip_start_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "clipStartPos");
  position_serialize_to_json (
    doc, clip_start_pos_obj, &ao->clip_start_pos, error);
  yyjson_mut_val * loop_start_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "loopStartPos");
  position_serialize_to_json (
    doc, loop_start_pos_obj, &ao->loop_start_pos, error);
  yyjson_mut_val * loop_end_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "loopEndPos");
  position_serialize_to_json (doc, loop_end_pos_obj, &ao->loop_end_pos, error);
  yyjson_mut_val * fade_in_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeInPos");
  position_serialize_to_json (doc, fade_in_pos_obj, &ao->fade_in_pos, error);
  yyjson_mut_val * fade_out_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeOutPos");
  position_serialize_to_json (doc, fade_out_pos_obj, &ao->fade_out_pos, error);
  yyjson_mut_val * fade_in_opts_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeInOpts");
  curve_options_serialize_to_json (
    doc, fade_in_opts_obj, &ao->fade_in_opts, error);
  yyjson_mut_val * fade_out_opts_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeOutOpts");
  curve_options_serialize_to_json (
    doc, fade_out_opts_obj, &ao->fade_out_opts, error);
  yyjson_mut_val * region_id_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "regionId");
  region_identifier_serialize_to_json (
    doc, region_id_obj, &ao->region_id, error);
  return true;
}

bool
region_identifier_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         id_obj,
  const RegionIdentifier * id,
  GError **                error)
{
  yyjson_mut_obj_add_int (doc, id_obj, "type", (int64_t) id->type);
  yyjson_mut_obj_add_int (doc, id_obj, "linkGroup", id->link_group);
  yyjson_mut_obj_add_uint (doc, id_obj, "trackNameHash", id->track_name_hash);
  yyjson_mut_obj_add_int (doc, id_obj, "lanePos", id->lane_pos);
  yyjson_mut_obj_add_int (doc, id_obj, "automationTrackIndex", id->at_idx);
  yyjson_mut_obj_add_int (doc, id_obj, "index", id->idx);
  return true;
}

static bool
velocity_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * vel_obj,
  const Velocity * vel,
  GError **        error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, vel_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &vel->base, error);
  yyjson_mut_obj_add_uint (doc, vel_obj, "velocity", vel->vel);
  return true;
}

bool
midi_note_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * mn_obj,
  const MidiNote * mn,
  GError **        error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, mn_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &mn->base, error);
  yyjson_mut_val * vel_obj = yyjson_mut_obj_add_obj (doc, mn_obj, "velocity");
  velocity_serialize_to_json (doc, vel_obj, mn->vel, error);
  yyjson_mut_obj_add_uint (doc, mn_obj, "value", mn->val);
  yyjson_mut_obj_add_bool (doc, mn_obj, "muted", mn->muted);
  yyjson_mut_obj_add_int (doc, mn_obj, "pos", mn->pos);
  return true;
}

bool
automation_point_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        ap_obj,
  const AutomationPoint * ap,
  GError **               error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, ap_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &ap->base, error);
  yyjson_mut_obj_add_real (doc, ap_obj, "fValue", ap->fvalue);
  yyjson_mut_obj_add_real (doc, ap_obj, "normalizedValue", ap->normalized_val);
  yyjson_mut_obj_add_int (doc, ap_obj, "index", ap->index);
  yyjson_mut_val * curve_opts_obj =
    yyjson_mut_obj_add_obj (doc, ap_obj, "curveOpts");
  curve_options_serialize_to_json (doc, curve_opts_obj, &ap->curve_opts, error);
  return true;
}

bool
chord_object_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    co_obj,
  const ChordObject * co,
  GError **           error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, co_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &co->base, error);
  yyjson_mut_obj_add_int (doc, co_obj, "index", co->index);
  yyjson_mut_obj_add_int (doc, co_obj, "chordIndex", co->chord_index);
  return true;
}

bool
region_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * r_obj,
  const ZRegion *  r,
  GError **        error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, r_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &r->base, error);
  yyjson_mut_val * id_obj = yyjson_mut_obj_add_obj (doc, r_obj, "id");
  region_identifier_serialize_to_json (doc, id_obj, &r->id, error);
  yyjson_mut_obj_add_str (doc, r_obj, "name", r->name);
  yyjson_mut_obj_add_int (doc, r_obj, "poolId", r->pool_id);
  yyjson_mut_obj_add_real (doc, r_obj, "gain", r->gain);
  yyjson_mut_val * color_obj = yyjson_mut_obj_add_obj (doc, r_obj, "color");
  gdk_rgba_serialize_to_json (doc, color_obj, &r->color, error);
  yyjson_mut_obj_add_bool (doc, r_obj, "useColor", r->use_color);
  switch (r->id.type)
    {
    case RegionType::REGION_TYPE_AUDIO:
      break;
    case RegionType::REGION_TYPE_MIDI:
      {
        yyjson_mut_val * midi_notes_arr =
          yyjson_mut_obj_add_arr (doc, r_obj, "midiNotes");
        for (int i = 0; i < r->num_midi_notes; i++)
          {
            MidiNote *       mn = r->midi_notes[i];
            yyjson_mut_val * mn_obj =
              yyjson_mut_arr_add_obj (doc, midi_notes_arr);
            midi_note_serialize_to_json (doc, mn_obj, mn, error);
          }
      }
      break;
    case RegionType::REGION_TYPE_CHORD:
      {
        yyjson_mut_val * chord_objects_arr =
          yyjson_mut_obj_add_arr (doc, r_obj, "chordObjects");
        for (int i = 0; i < r->num_chord_objects; i++)
          {
            ChordObject *    co = r->chord_objects[i];
            yyjson_mut_val * co_obj =
              yyjson_mut_arr_add_obj (doc, chord_objects_arr);
            chord_object_serialize_to_json (doc, co_obj, co, error);
          }
      }
      break;
    case RegionType::REGION_TYPE_AUTOMATION:
      {
        yyjson_mut_val * automation_points_arr =
          yyjson_mut_obj_add_arr (doc, r_obj, "automationPoints");
        for (int i = 0; i < r->num_aps; i++)
          {
            AutomationPoint * ap = r->aps[i];
            yyjson_mut_val *  ap_obj =
              yyjson_mut_arr_add_obj (doc, automation_points_arr);
            automation_point_serialize_to_json (doc, ap_obj, ap, error);
          }
      }
      break;
    }
  return true;
}

static bool
musical_scale_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     scale_obj,
  const MusicalScale * scale,
  GError **            error)
{
  yyjson_mut_obj_add_int (doc, scale_obj, "type", (int64_t) scale->type);
  yyjson_mut_obj_add_int (doc, scale_obj, "rootKey", (int64_t) scale->root_key);
  return true;
}

bool
scale_object_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    so_obj,
  const ScaleObject * so,
  GError **           error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, so_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &so->base, error);
  yyjson_mut_obj_add_int (doc, so_obj, "index", so->index);
  yyjson_mut_val * scale_obj = yyjson_mut_obj_add_obj (doc, so_obj, "scale");
  musical_scale_serialize_to_json (doc, scale_obj, so->scale, error);
  return true;
}

bool
marker_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * m_obj,
  const Marker *   m,
  GError **        error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, m_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &m->base, error);
  yyjson_mut_obj_add_str (doc, m_obj, "name", m->name);
  yyjson_mut_obj_add_uint (doc, m_obj, "trackNameHash", m->track_name_hash);
  yyjson_mut_obj_add_int (doc, m_obj, "index", m->index);
  yyjson_mut_obj_add_int (doc, m_obj, "type", (int64_t) m->type);
  return true;
}

bool
arranger_object_deserialize_from_json (
  yyjson_doc *     doc,
  yyjson_val *     ao_obj,
  ArrangerObject * ao,
  GError **        error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (ao_obj);
  ao->type =
    (ArrangerObjectType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  ao->flags =
    (ArrangerObjectFlags) yyjson_get_int (yyjson_obj_iter_get (&it, "flags"));
  ao->muted = yyjson_get_bool (yyjson_obj_iter_get (&it, "muted"));
  yyjson_val * pos_obj = yyjson_obj_iter_get (&it, "pos");
  position_deserialize_from_json (doc, pos_obj, &ao->pos, error);
  yyjson_val * end_pos_obj = yyjson_obj_iter_get (&it, "endPos");
  position_deserialize_from_json (doc, end_pos_obj, &ao->end_pos, error);
  yyjson_val * clip_start_pos_obj = yyjson_obj_iter_get (&it, "clipStartPos");
  position_deserialize_from_json (
    doc, clip_start_pos_obj, &ao->clip_start_pos, error);
  yyjson_val * loop_start_pos_obj = yyjson_obj_iter_get (&it, "loopStartPos");
  position_deserialize_from_json (
    doc, loop_start_pos_obj, &ao->loop_start_pos, error);
  yyjson_val * loop_end_pos_obj = yyjson_obj_iter_get (&it, "loopEndPos");
  position_deserialize_from_json (
    doc, loop_end_pos_obj, &ao->loop_end_pos, error);
  yyjson_val * fade_in_pos_obj = yyjson_obj_iter_get (&it, "fadeInPos");
  position_deserialize_from_json (doc, fade_in_pos_obj, &ao->fade_in_pos, error);
  yyjson_val * fade_out_pos_obj = yyjson_obj_iter_get (&it, "fadeOutPos");
  position_deserialize_from_json (
    doc, fade_out_pos_obj, &ao->fade_out_pos, error);
  yyjson_val * fade_in_opts_obj = yyjson_obj_iter_get (&it, "fadeInOpts");
  curve_options_deserialize_from_json (
    doc, fade_in_opts_obj, &ao->fade_in_opts, error);
  yyjson_val * fade_out_opts_obj = yyjson_obj_iter_get (&it, "fadeOutOpts");
  curve_options_deserialize_from_json (
    doc, fade_out_opts_obj, &ao->fade_out_opts, error);
  yyjson_val * region_id_obj = yyjson_obj_iter_get (&it, "regionId");
  region_identifier_deserialize_from_json (
    doc, region_id_obj, &ao->region_id, error);
  ao->magic = ARRANGER_OBJECT_MAGIC;
  return true;
}

bool
region_identifier_deserialize_from_json (
  yyjson_doc *       doc,
  yyjson_val *       id_obj,
  RegionIdentifier * id,
  GError **          error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (id_obj);
  id->type = (RegionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  id->link_group = yyjson_get_int (yyjson_obj_iter_get (&it, "linkGroup"));
  id->track_name_hash =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "trackNameHash"));
  id->lane_pos = yyjson_get_int (yyjson_obj_iter_get (&it, "lanePos"));
  id->at_idx =
    yyjson_get_int (yyjson_obj_iter_get (&it, "automationTrackIndex"));
  id->idx = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  return true;
}

static bool
velocity_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * vel_obj,
  Velocity *   vel,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (vel_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_object_deserialize_from_json (doc, base_obj, &vel->base, error);
  vel->vel = (uint8_t) yyjson_get_uint (yyjson_obj_iter_get (&it, "velocity"));
  return true;
}

bool
midi_note_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * mn_obj,
  MidiNote *   mn,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (mn_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_object_deserialize_from_json (doc, base_obj, &mn->base, error);
  yyjson_val * vel_obj = yyjson_obj_iter_get (&it, "velocity");
  mn->vel = object_new (Velocity);
  velocity_deserialize_from_json (doc, vel_obj, mn->vel, error);
  mn->val = yyjson_get_uint (yyjson_obj_iter_get (&it, "value"));
  mn->muted = yyjson_get_bool (yyjson_obj_iter_get (&it, "muted"));
  mn->pos = yyjson_get_int (yyjson_obj_iter_get (&it, "pos"));
  mn->magic = MIDI_NOTE_MAGIC;
  return true;
}

bool
automation_point_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      ap_obj,
  AutomationPoint * ap,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (ap_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_object_deserialize_from_json (doc, base_obj, &ap->base, error);
  ap->fvalue = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "fValue"));
  ap->normalized_val =
    (float) yyjson_get_real (yyjson_obj_iter_get (&it, "normalizedValue"));
  ap->index = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  yyjson_val * curve_opts_obj = yyjson_obj_iter_get (&it, "curveOpts");
  curve_options_deserialize_from_json (
    doc, curve_opts_obj, &ap->curve_opts, error);
  return true;
}

bool
chord_object_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  co_obj,
  ChordObject * co,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (co_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_object_deserialize_from_json (doc, base_obj, &co->base, error);
  co->index = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  /* this was missing until mid-1.9 */
  yyjson_val * chord_index_val = yyjson_obj_iter_get (&it, "chordIndex");
  if (chord_index_val)
    {
      co->chord_index = yyjson_get_int (chord_index_val);
    }
  co->magic = CHORD_OBJECT_MAGIC;
  return true;
}

bool
region_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * r_obj,
  ZRegion *    r,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (r_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_object_deserialize_from_json (doc, base_obj, &r->base, error);
  yyjson_val * id_obj = yyjson_obj_iter_get (&it, "id");
  region_identifier_deserialize_from_json (doc, id_obj, &r->id, error);
  r->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  r->pool_id = yyjson_get_int (yyjson_obj_iter_get (&it, "poolId"));
  r->gain = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "gain"));
  yyjson_val * color_obj = yyjson_obj_iter_get (&it, "color");
  gdk_rgba_deserialize_from_json (doc, color_obj, &r->color, error);
  r->use_color = yyjson_get_bool (yyjson_obj_iter_get (&it, "useColor"));
  switch (r->id.type)
    {
    case RegionType::REGION_TYPE_AUDIO:
      break;
    case RegionType::REGION_TYPE_MIDI:
      {
        yyjson_val * midi_notes_arr = yyjson_obj_iter_get (&it, "midiNotes");
        r->midi_notes_size = yyjson_arr_size (midi_notes_arr);
        if (r->midi_notes_size > 0)
          {
            r->midi_notes = object_new_n (r->midi_notes_size, MidiNote *);
            yyjson_arr_iter midi_note_it = yyjson_arr_iter_with (midi_notes_arr);
            yyjson_val * mn_obj = NULL;
            while ((mn_obj = yyjson_arr_iter_next (&midi_note_it)))
              {
                MidiNote * mn = object_new (MidiNote);
                r->midi_notes[r->num_midi_notes++] = mn;
                midi_note_deserialize_from_json (doc, mn_obj, mn, error);
              }
          }
      }
      break;
    case RegionType::REGION_TYPE_CHORD:
      {
        yyjson_val * chord_objects_arr =
          yyjson_obj_iter_get (&it, "chordObjects");
        r->chord_objects_size = yyjson_arr_size (chord_objects_arr);
        if (r->chord_objects_size > 0)
          {
            r->chord_objects =
              object_new_n (r->chord_objects_size, ChordObject *);
            yyjson_arr_iter chord_object_it =
              yyjson_arr_iter_with (chord_objects_arr);
            yyjson_val * co_obj = NULL;
            while ((co_obj = yyjson_arr_iter_next (&chord_object_it)))
              {
                ChordObject * co = object_new (ChordObject);
                r->chord_objects[r->num_chord_objects++] = co;
                chord_object_deserialize_from_json (doc, co_obj, co, error);
              }
          }
      }
      break;
    case RegionType::REGION_TYPE_AUTOMATION:
      {
        yyjson_val * automation_points_arr =
          yyjson_obj_iter_get (&it, "automationPoints");
        r->aps_size = yyjson_arr_size (automation_points_arr);
        if (r->aps_size > 0)
          {
            r->aps = object_new_n (r->aps_size, AutomationPoint *);
            yyjson_arr_iter automation_point_it =
              yyjson_arr_iter_with (automation_points_arr);
            yyjson_val * ap_obj = NULL;
            while ((ap_obj = yyjson_arr_iter_next (&automation_point_it)))
              {
                AutomationPoint * ap = object_new (AutomationPoint);
                r->aps[r->num_aps++] = ap;
                automation_point_deserialize_from_json (doc, ap_obj, ap, error);
              }
          }
      }
      break;
    }
  r->magic = REGION_MAGIC;
  return true;
}

static bool
musical_scale_deserialize_from_json (
  yyjson_doc *   doc,
  yyjson_val *   scale_obj,
  MusicalScale * scale,
  GError **      error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (scale_obj);
  scale->type =
    (MusicalScaleType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  scale->root_key =
    (MusicalNote) yyjson_get_int (yyjson_obj_iter_get (&it, "rootKey"));
  return true;
}

bool
scale_object_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  so_obj,
  ScaleObject * so,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (so_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_object_deserialize_from_json (doc, base_obj, &so->base, error);
  so->index = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  yyjson_val * scale_obj = yyjson_obj_iter_get (&it, "scale");
  so->scale = object_new (MusicalScale);
  musical_scale_deserialize_from_json (doc, scale_obj, so->scale, error);
  return true;
}

bool
marker_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * m_obj,
  Marker *     m,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (m_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  arranger_object_deserialize_from_json (doc, base_obj, &m->base, error);
  m->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  m->track_name_hash =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "trackNameHash"));
  m->index = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  m->type = (MarkerType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  return true;
}
