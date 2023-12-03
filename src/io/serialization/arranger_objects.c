// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/marker.h"
#include "dsp/region.h"
#include "dsp/scale_object.h"
#include "gui/backend/arranger_object.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/extra.h"

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
  yyjson_mut_obj_add_int (doc, ao_obj, "type", ao->type);
  yyjson_mut_obj_add_int (doc, ao_obj, "flags", ao->flags);
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
  yyjson_mut_obj_add_int (doc, id_obj, "type", id->type);
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
#if 0
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, vel_obj, "base");
  arranger_object_serialize_to_json (doc, base_obj, &vel->base, error);
#endif
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
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_MIDI:
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
    case REGION_TYPE_CHORD:
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
    case REGION_TYPE_AUTOMATION:
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
  yyjson_mut_obj_add_int (doc, scale_obj, "type", scale->type);
  yyjson_mut_obj_add_int (doc, scale_obj, "rootKey", scale->root_key);
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
  yyjson_mut_obj_add_int (doc, m_obj, "type", m->type);
  return true;
}
