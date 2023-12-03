// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "io/serialization/actions.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/engine.h"
#include "io/serialization/extra.h"
#include "io/serialization/gui_backend.h"
#include "io/serialization/port.h"
#include "io/serialization/project.h"
#include "io/serialization/selections.h"
#include "io/serialization/track.h"
#include "project.h"

#include <yyjson.h>

typedef enum
{
  Z_IO_SERIALIZATION_PROJECT_ERROR_FAILED,
} ZIOSerializationProjectError;

#define Z_IO_SERIALIZATION_PROJECT_ERROR \
  z_io_serialization_project_error_quark ()
GQuark
z_io_serialization_project_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - project - error - quark, z_io_serialization_project_error)

static bool
region_link_group_manager_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               mgr_obj,
  const RegionLinkGroupManager * mgr,
  GError **                      error)
{
  if (mgr->groups)
    {
      yyjson_mut_val * groups_arr =
        yyjson_mut_obj_add_arr (doc, mgr_obj, "groups");
      for (int i = 0; i < mgr->num_groups; i++)
        {
          RegionLinkGroup * group = mgr->groups[i];
          yyjson_mut_val * group_obj = yyjson_mut_arr_add_obj (doc, groups_arr);
          yyjson_mut_val * ids_arr =
            yyjson_mut_obj_add_arr (doc, group_obj, "ids");
          for (int j = 0; j < group->num_ids; j++)
            {
              RegionIdentifier * id = &group->ids[j];
              yyjson_mut_val *   id_obj = yyjson_mut_arr_add_obj (doc, ids_arr);
              region_identifier_serialize_to_json (doc, id_obj, id, error);
            }
          yyjson_mut_obj_add_int (doc, group_obj, "groupIdx", group->group_idx);
        }
    }
  return true;
}

static bool
midi_mappings_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     mm_obj,
  const MidiMappings * mm,
  GError **            error)
{
  if (mm->mappings)
    {
      yyjson_mut_val * mappings_arr =
        yyjson_mut_obj_add_arr (doc, mm_obj, "mappings");
      for (int i = 0; i < mm->num_mappings; i++)
        {
          MidiMapping *    m = mm->mappings[i];
          yyjson_mut_val * m_obj = yyjson_mut_arr_add_obj (doc, mappings_arr);
          yyjson_mut_val * key_obj = yyjson_mut_arr_with_uint8 (doc, m->key, 3);
          yyjson_mut_obj_add_val (doc, m_obj, "key", key_obj);
          if (m->device_port)
            {
              yyjson_mut_val * device_port_obj =
                yyjson_mut_obj_add_obj (doc, m_obj, "devicePort");
              ext_port_serialize_to_json (
                doc, device_port_obj, m->device_port, error);
            }
          yyjson_mut_val * dest_id_obj =
            yyjson_mut_obj_add_obj (doc, m_obj, "destId");
          port_identifier_serialize_to_json (
            doc, dest_id_obj, &m->dest_id, error);
          yyjson_mut_obj_add_bool (doc, m_obj, "enabled", m->enabled);
        }
    }
  return true;
}

static bool
undo_stack_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  stack_obj,
  const UndoStack * stack,
  GError **         error)
{
  if (stack->as_actions)
    {
      yyjson_mut_val * as_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "arrangerSelectionsActions");
      for (size_t i = 0; i < stack->num_as_actions; i++)
        {
          yyjson_mut_val * as_action_obj =
            yyjson_mut_arr_add_obj (doc, as_actions_arr);
          arranger_selections_action_serialize_to_json (
            doc, as_action_obj, stack->as_actions[i], error);
        }
    }
  if (stack->mixer_selections_actions)
    {
      yyjson_mut_val * mixer_selections_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "mixerSelectionsActions");
      for (size_t i = 0; i < stack->num_mixer_selections_actions; i++)
        {
          yyjson_mut_val * mixer_selections_action_obj =
            yyjson_mut_arr_add_obj (doc, mixer_selections_actions_arr);
          mixer_selections_action_serialize_to_json (
            doc, mixer_selections_action_obj,
            stack->mixer_selections_actions[i], error);
        }
    }
  if (stack->tracklist_selections_actions)
    {
      yyjson_mut_val * tracklist_selections_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_tracklist_selections_actions; i++)
        {
          yyjson_mut_val * tracklist_selections_action_obj =
            yyjson_mut_arr_add_obj (doc, tracklist_selections_actions_arr);
          tracklist_selections_action_serialize_to_json (
            doc, tracklist_selections_action_obj,
            stack->tracklist_selections_actions[i], error);
        }
    }
  if (stack->channel_send_actions)
    {
      yyjson_mut_val * channel_send_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_channel_send_actions; i++)
        {
          yyjson_mut_val * channel_send_action_obj =
            yyjson_mut_arr_add_obj (doc, channel_send_actions_arr);
          channel_send_action_serialize_to_json (
            doc, channel_send_action_obj, stack->channel_send_actions[i], error);
        }
    }
  if (stack->port_connection_actions)
    {
      yyjson_mut_val * port_connection_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_port_connection_actions; i++)
        {
          yyjson_mut_val * port_connection_action_obj =
            yyjson_mut_arr_add_obj (doc, port_connection_actions_arr);
          port_connection_action_serialize_to_json (
            doc, port_connection_action_obj, stack->port_connection_actions[i],
            error);
        }
    }
  if (stack->port_actions)
    {
      yyjson_mut_val * port_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_port_actions; i++)
        {
          yyjson_mut_val * port_action_obj =
            yyjson_mut_arr_add_obj (doc, port_actions_arr);
          port_action_serialize_to_json (
            doc, port_action_obj, stack->port_actions[i], error);
        }
    }
  if (stack->midi_mapping_actions)
    {
      yyjson_mut_val * midi_mapping_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_midi_mapping_actions; i++)
        {
          yyjson_mut_val * midi_mapping_action_obj =
            yyjson_mut_arr_add_obj (doc, midi_mapping_actions_arr);
          midi_mapping_action_serialize_to_json (
            doc, midi_mapping_action_obj, stack->midi_mapping_actions[i], error);
        }
    }
  if (stack->range_actions)
    {
      yyjson_mut_val * range_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_range_actions; i++)
        {
          yyjson_mut_val * range_action_obj =
            yyjson_mut_arr_add_obj (doc, range_actions_arr);
          range_action_serialize_to_json (
            doc, range_action_obj, stack->range_actions[i], error);
        }
    }
  if (stack->transport_actions)
    {
      yyjson_mut_val * transport_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_transport_actions; i++)
        {
          yyjson_mut_val * transport_action_obj =
            yyjson_mut_arr_add_obj (doc, transport_actions_arr);
          transport_action_serialize_to_json (
            doc, transport_action_obj, stack->transport_actions[i], error);
        }
    }
  if (stack->chord_actions)
    {
      yyjson_mut_val * chord_actions_arr =
        yyjson_mut_obj_add_arr (doc, stack_obj, "tracklistSelectionsActions");
      for (size_t i = 0; i < stack->num_chord_actions; i++)
        {
          yyjson_mut_val * chord_action_obj =
            yyjson_mut_arr_add_obj (doc, chord_actions_arr);
          chord_action_serialize_to_json (
            doc, chord_action_obj, stack->chord_actions[i], error);
        }
    }
  yyjson_mut_val * s_obj = yyjson_mut_obj_add_obj (doc, stack_obj, "stack");
  stack_serialize_to_json (doc, s_obj, stack->stack, error);
  return true;
}

static bool
undo_manager_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    um_obj,
  const UndoManager * um,
  GError **           error)
{
  yyjson_mut_val * undo_stack_obj =
    yyjson_mut_obj_add_obj (doc, um_obj, "undoStack");
  undo_stack_serialize_to_json (doc, undo_stack_obj, um->undo_stack, error);
  yyjson_mut_val * redo_stack_obj =
    yyjson_mut_obj_add_obj (doc, um_obj, "redoStack");
  undo_stack_serialize_to_json (doc, redo_stack_obj, um->redo_stack, error);
  return true;
}

char *
project_serialize_to_json_str (const Project * prj, GError ** error)
{
  /* create a mutable doc */
  yyjson_mut_doc * doc = yyjson_mut_doc_new (NULL);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  if (!root)
    {
      g_set_error_literal (
        error, Z_IO_SERIALIZATION_PROJECT_ERROR,
        Z_IO_SERIALIZATION_PROJECT_ERROR_FAILED, "Failed to create root obj");
      return NULL;
    }
  yyjson_mut_doc_set_root (doc, root);

  /* format 1.6 */
  yyjson_mut_obj_add_int (doc, root, "formatMajor", 1);
  yyjson_mut_obj_add_int (doc, root, "formatMinor", 6);
  yyjson_mut_obj_add_str (doc, root, "type", "ZrythmProject");

  yyjson_mut_obj_add_str (doc, root, "title", prj->title);
  yyjson_mut_obj_add_str (doc, root, "datetime", prj->datetime_str);
  yyjson_mut_obj_add_str (doc, root, "version", prj->version);

  /* tracklist */
  Tracklist *      tracklist = prj->tracklist;
  yyjson_mut_val * tracklist_obj =
    yyjson_mut_obj_add_obj (doc, root, "tracklist");
  yyjson_mut_obj_add_int (
    doc, tracklist_obj, "pinnedTracksCutoff", tracklist->pinned_tracks_cutoff);
  yyjson_mut_val * tracks_arr =
    yyjson_mut_obj_add_arr (doc, tracklist_obj, "tracks");
  for (int i = 0; i < tracklist->num_tracks; i++)
    {
      Track *          track = tracklist->tracks[i];
      yyjson_mut_val * track_obj = yyjson_mut_arr_add_obj (doc, tracks_arr);
      track_serialize_to_json (doc, track_obj, track, error);
    }
  yyjson_mut_val * clip_editor_obj =
    yyjson_mut_obj_add_obj (doc, root, "clipEditor");
  clip_editor_serialize_to_json (doc, clip_editor_obj, prj->clip_editor, error);
  yyjson_mut_val * timeline_obj = yyjson_mut_obj_add_obj (doc, root, "timeline");
  timeline_serialize_to_json (doc, timeline_obj, prj->timeline, error);
  yyjson_mut_val * snap_grid_tl_obj =
    yyjson_mut_obj_add_obj (doc, root, "snapGridTimeline");
  snap_grid_serialize_to_json (
    doc, snap_grid_tl_obj, prj->snap_grid_timeline, error);
  yyjson_mut_val * snap_grid_editor_obj =
    yyjson_mut_obj_add_obj (doc, root, "snapGridEditor");
  snap_grid_serialize_to_json (
    doc, snap_grid_editor_obj, prj->snap_grid_editor, error);
  yyjson_mut_val * quantize_options_tl_obj =
    yyjson_mut_obj_add_obj (doc, root, "quantizeOptsTimeline");
  quantize_options_serialize_to_json (
    doc, quantize_options_tl_obj, prj->quantize_opts_timeline, error);
  yyjson_mut_val * quantize_options_editor_obj =
    yyjson_mut_obj_add_obj (doc, root, "quantizeOptsEditor");
  quantize_options_serialize_to_json (
    doc, quantize_options_editor_obj, prj->quantize_opts_editor, error);
  yyjson_mut_val * audio_engine_obj =
    yyjson_mut_obj_add_obj (doc, root, "audioEngine");
  audio_engine_serialize_to_json (
    doc, audio_engine_obj, prj->audio_engine, error);
  yyjson_mut_val * mixer_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "mixerSelections");
  mixer_selections_serialize_to_json (
    doc, mixer_selections_obj, prj->mixer_selections, error);
  yyjson_mut_val * timeline_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "timelineSelections");
  timeline_selections_serialize_to_json (
    doc, timeline_selections_obj, prj->timeline_selections, error);
  yyjson_mut_val * midi_arranger_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "midiArrangerSelections");
  midi_arranger_selections_serialize_to_json (
    doc, midi_arranger_selections_obj, prj->midi_arranger_selections, error);
  yyjson_mut_val * chord_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "chordSelections");
  chord_selections_serialize_to_json (
    doc, chord_selections_obj, prj->chord_selections, error);
  yyjson_mut_val * automation_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "automationSelections");
  automation_selections_serialize_to_json (
    doc, automation_selections_obj, prj->automation_selections, error);
  yyjson_mut_val * audio_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "audioSelections");
  audio_selections_serialize_to_json (
    doc, audio_selections_obj, prj->audio_selections, error);
  yyjson_mut_val * tracklist_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "tracklistSelections");
  tracklist_selections_serialize_to_json (
    doc, tracklist_selections_obj, prj->tracklist_selections, error);
  yyjson_mut_val * region_link_group_mgr_obj =
    yyjson_mut_obj_add_obj (doc, root, "regionLinkGroupManager");
  region_link_group_manager_serialize_to_json (
    doc, region_link_group_mgr_obj, prj->region_link_group_manager, error);
  yyjson_mut_val * port_connections_mgr_obj =
    yyjson_mut_obj_add_obj (doc, root, "portConnectionsManager");
  port_connections_manager_serialize_to_json (
    doc, port_connections_mgr_obj, prj->port_connections_manager, error);
  yyjson_mut_val * midi_mappings_obj =
    yyjson_mut_obj_add_obj (doc, root, "midiMappings");
  midi_mappings_serialize_to_json (
    doc, midi_mappings_obj, prj->midi_mappings, error);
  if (prj->undo_manager)
    {
      yyjson_mut_val * undo_manager_obj =
        yyjson_mut_obj_add_obj (doc, root, "undoManager");
      undo_manager_serialize_to_json (
        doc, undo_manager_obj, prj->undo_manager, error);
    }
  yyjson_mut_obj_add_int (doc, root, "lastSelection", prj->last_selection);

  /* to string - minified */
  char * json = yyjson_mut_write (
    doc, YYJSON_WRITE_NOFLAG /* YYJSON_WRITE_PRETTY_TWO_SPACES */, NULL);
  g_message ("done writing json to string");
  if (json)
    {
      /*g_message ("json: %s\n", json);*/
    }

  yyjson_mut_doc_free (doc);
  G_BREAKPOINT ();

  return json;
}
