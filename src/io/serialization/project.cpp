// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tracklist.h"
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
#include "utils/objects.h"

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
        yyjson_mut_obj_add_arr (doc, stack_obj, "channelSendActions");
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
        yyjson_mut_obj_add_arr (doc, stack_obj, "portConnectionActions");
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
        yyjson_mut_obj_add_arr (doc, stack_obj, "portActions");
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
        yyjson_mut_obj_add_arr (doc, stack_obj, "midiMappingActions");
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
        yyjson_mut_obj_add_arr (doc, stack_obj, "rangeActions");
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
        yyjson_mut_obj_add_arr (doc, stack_obj, "transportActions");
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
        yyjson_mut_obj_add_arr (doc, stack_obj, "chordActions");
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

static bool
region_link_group_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      group_obj,
  RegionLinkGroup * group,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (group_obj);
  yyjson_val *    ids_arr = yyjson_obj_iter_get (&it, "ids");
  group->ids_size = yyjson_arr_size (ids_arr);
  if (group->ids_size > 0)
    {
      group->ids = object_new_n (group->ids_size, RegionIdentifier);
      yyjson_arr_iter ids_it = yyjson_arr_iter_with (ids_arr);
      yyjson_val *    id_obj = NULL;
      while ((id_obj = yyjson_arr_iter_next (&ids_it)))
        {
          region_identifier_deserialize_from_json (
            doc, id_obj, &group->ids[group->num_ids++], error);
        }
    }
  group->group_idx = yyjson_get_int (yyjson_obj_iter_get (&it, "groupIdx"));
  return true;
}

static bool
region_link_group_manager_deserialize_from_json (
  yyjson_doc *             doc,
  yyjson_val *             mgr_obj,
  RegionLinkGroupManager * mgr,
  GError **                error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (mgr_obj);
  yyjson_val *    groups_arr = yyjson_obj_iter_get (&it, "groups");
  if (groups_arr)
    {
      mgr->groups_size = yyjson_arr_size (groups_arr);
      if (mgr->groups_size > 0)
        {
          mgr->groups = object_new_n (mgr->groups_size, RegionLinkGroup *);
          yyjson_arr_iter groups_it = yyjson_arr_iter_with (groups_arr);
          yyjson_val *    group_obj = NULL;
          while ((group_obj = yyjson_arr_iter_next (&groups_it)))
            {
              RegionLinkGroup * group = object_new (RegionLinkGroup);
              mgr->groups[mgr->num_groups++] = group;
              region_link_group_deserialize_from_json (
                doc, group_obj, group, error);
            }
        }
    }
  return true;
}

static bool
midi_mapping_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  m_obj,
  MidiMapping * m,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (m_obj);
  yyjson_val *    key_arr = yyjson_obj_iter_get (&it, "key");
  yyjson_arr_iter key_it = yyjson_arr_iter_with (key_arr);
  yyjson_val *    key_element = NULL;
  size_t          count = 0;
  while ((key_element = yyjson_arr_iter_next (&key_it)))
    {
      m->key[count++] = (uint8_t) yyjson_get_uint (key_element);
    }
  yyjson_val * device_port_obj = yyjson_obj_iter_get (&it, "devicePort");
  if (device_port_obj)
    {
      m->device_port = object_new (ExtPort);
      ext_port_deserialize_from_json (
        doc, device_port_obj, m->device_port, error);
    }
  yyjson_val * dest_id_obj = yyjson_obj_iter_get (&it, "destId");
  port_identifier_deserialize_from_json (doc, dest_id_obj, &m->dest_id, error);
  m->enabled = yyjson_get_bool (yyjson_obj_iter_get (&it, "enabled"));
  return true;
}

static bool
midi_mappings_deserialize_from_json (
  yyjson_doc *   doc,
  yyjson_val *   mm_obj,
  MidiMappings * mm,
  GError **      error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (mm_obj);
  yyjson_val *    mappings_arr = yyjson_obj_iter_get (&it, "mappings");
  mm->mappings_size = yyjson_arr_size (mappings_arr);
  if (mm->mappings_size > 0)
    {
      mm->mappings = object_new_n (mm->mappings_size, MidiMapping *);
      yyjson_arr_iter mappings_it = yyjson_arr_iter_with (mappings_arr);
      yyjson_val *    mapping_obj = NULL;
      while ((mapping_obj = yyjson_arr_iter_next (&mappings_it)))
        {
          MidiMapping * mapping = new MidiMapping ();
          mm->mappings[mm->num_mappings++] = mapping;
          midi_mapping_deserialize_from_json (doc, mapping_obj, mapping, error);
        }
    }
  return true;
}

static bool
undo_stack_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * stack_obj,
  UndoStack *  stack,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (stack_obj);
  yyjson_val *    as_actions_arr =
    yyjson_obj_iter_get (&it, "arrangerSelectionsActions");
  if (as_actions_arr)
    {
      stack->as_actions_size = yyjson_arr_size (as_actions_arr);
      if (stack->as_actions_size > 0)
        {
          stack->as_actions =
            object_new_n (stack->as_actions_size, ArrangerSelectionsAction *);
          yyjson_arr_iter as_actions_it = yyjson_arr_iter_with (as_actions_arr);
          yyjson_val *    action_obj = NULL;
          while ((action_obj = yyjson_arr_iter_next (&as_actions_it)))
            {
              ArrangerSelectionsAction * action =
                object_new (ArrangerSelectionsAction);
              stack->as_actions[stack->num_as_actions++] = action;
              arranger_selections_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * mixer_selections_actions_arr =
    yyjson_obj_iter_get (&it, "mixerSelectionsActions");
  if (mixer_selections_actions_arr)
    {
      stack->mixer_selections_actions_size =
        yyjson_arr_size (mixer_selections_actions_arr);
      if (stack->mixer_selections_actions_size > 0)
        {
          stack->mixer_selections_actions = object_new_n (
            stack->mixer_selections_actions_size, MixerSelectionsAction *);
          yyjson_arr_iter mixer_selections_actions_it =
            yyjson_arr_iter_with (mixer_selections_actions_arr);
          yyjson_val * action_obj = NULL;
          while (
            (action_obj = yyjson_arr_iter_next (&mixer_selections_actions_it)))
            {
              MixerSelectionsAction * action =
                object_new (MixerSelectionsAction);
              stack->mixer_selections_actions
                [stack->num_mixer_selections_actions++] = action;
              mixer_selections_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * tracklist_selections_actions_arr =
    yyjson_obj_iter_get (&it, "tracklistSelectionsActions");
  if (tracklist_selections_actions_arr)
    {
      stack->tracklist_selections_actions_size =
        yyjson_arr_size (tracklist_selections_actions_arr);
      if (stack->tracklist_selections_actions_size > 0)
        {
          stack->tracklist_selections_actions = object_new_n (
            stack->tracklist_selections_actions_size,
            TracklistSelectionsAction *);
          yyjson_arr_iter tracklist_selections_actions_it =
            yyjson_arr_iter_with (tracklist_selections_actions_arr);
          yyjson_val * action_obj = NULL;
          while ((
            action_obj = yyjson_arr_iter_next (&tracklist_selections_actions_it)))
            {
              TracklistSelectionsAction * action =
                object_new (TracklistSelectionsAction);
              stack->tracklist_selections_actions
                [stack->num_tracklist_selections_actions++] = action;
              tracklist_selections_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * channel_send_actions_arr =
    yyjson_obj_iter_get (&it, "channelSendActions");
  if (channel_send_actions_arr)
    {
      stack->channel_send_actions_size =
        yyjson_arr_size (channel_send_actions_arr);
      if (stack->channel_send_actions_size > 0)
        {
          stack->channel_send_actions = object_new_n (
            stack->channel_send_actions_size, ChannelSendAction *);
          yyjson_arr_iter channel_send_actions_it =
            yyjson_arr_iter_with (channel_send_actions_arr);
          yyjson_val * action_obj = NULL;
          while ((action_obj = yyjson_arr_iter_next (&channel_send_actions_it)))
            {
              ChannelSendAction * action = object_new (ChannelSendAction);
              stack->channel_send_actions[stack->num_channel_send_actions++] =
                action;
              channel_send_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * port_connection_actions_arr =
    yyjson_obj_iter_get (&it, "portConnectionActions");
  if (port_connection_actions_arr)
    {
      stack->port_connection_actions_size =
        yyjson_arr_size (port_connection_actions_arr);
      if (stack->port_connection_actions_size > 0)
        {
          stack->port_connection_actions = object_new_n (
            stack->port_connection_actions_size, PortConnectionAction *);
          yyjson_arr_iter port_connection_actions_it =
            yyjson_arr_iter_with (port_connection_actions_arr);
          yyjson_val * action_obj = NULL;
          while (
            (action_obj = yyjson_arr_iter_next (&port_connection_actions_it)))
            {
              PortConnectionAction * action = object_new (PortConnectionAction);
              stack->port_connection_actions
                [stack->num_port_connection_actions++] = action;
              port_connection_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * port_actions_arr = yyjson_obj_iter_get (&it, "portActions");
  if (port_actions_arr)
    {
      stack->port_actions_size = yyjson_arr_size (port_actions_arr);
      if (stack->port_actions_size > 0)
        {
          stack->port_actions =
            object_new_n (stack->port_actions_size, PortAction *);
          yyjson_arr_iter port_actions_it =
            yyjson_arr_iter_with (port_actions_arr);
          yyjson_val * action_obj = NULL;
          while ((action_obj = yyjson_arr_iter_next (&port_actions_it)))
            {
              PortAction * action = object_new (PortAction);
              stack->port_actions[stack->num_port_actions++] = action;
              port_action_deserialize_from_json (doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * midi_mapping_actions_arr =
    yyjson_obj_iter_get (&it, "midiMappingActions");
  if (midi_mapping_actions_arr)
    {
      stack->midi_mapping_actions_size =
        yyjson_arr_size (midi_mapping_actions_arr);
      if (stack->midi_mapping_actions_size > 0)
        {
          stack->midi_mapping_actions = object_new_n (
            stack->midi_mapping_actions_size, MidiMappingAction *);
          yyjson_arr_iter midi_mapping_actions_it =
            yyjson_arr_iter_with (midi_mapping_actions_arr);
          yyjson_val * action_obj = NULL;
          while ((action_obj = yyjson_arr_iter_next (&midi_mapping_actions_it)))
            {
              MidiMappingAction * action = object_new (MidiMappingAction);
              stack->midi_mapping_actions[stack->num_midi_mapping_actions++] =
                action;
              midi_mapping_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * range_actions_arr = yyjson_obj_iter_get (&it, "rangeActions");
  if (range_actions_arr)
    {
      stack->range_actions_size = yyjson_arr_size (range_actions_arr);
      if (stack->range_actions_size > 0)
        {
          stack->range_actions =
            object_new_n (stack->range_actions_size, RangeAction *);
          yyjson_arr_iter range_actions_it =
            yyjson_arr_iter_with (range_actions_arr);
          yyjson_val * action_obj = NULL;
          while ((action_obj = yyjson_arr_iter_next (&range_actions_it)))
            {
              RangeAction * action = object_new (RangeAction);
              stack->range_actions[stack->num_range_actions++] = action;
              range_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * transport_actions_arr =
    yyjson_obj_iter_get (&it, "transportActions");
  if (transport_actions_arr)
    {
      stack->transport_actions_size = yyjson_arr_size (transport_actions_arr);
      if (stack->transport_actions_size > 0)
        {
          stack->transport_actions =
            object_new_n (stack->transport_actions_size, TransportAction *);
          yyjson_arr_iter transport_actions_it =
            yyjson_arr_iter_with (transport_actions_arr);
          yyjson_val * action_obj = NULL;
          while ((action_obj = yyjson_arr_iter_next (&transport_actions_it)))
            {
              TransportAction * action = object_new (TransportAction);
              stack->transport_actions[stack->num_transport_actions++] = action;
              transport_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * chord_actions_arr = yyjson_obj_iter_get (&it, "chordActions");
  if (chord_actions_arr)
    {
      stack->chord_actions_size = yyjson_arr_size (chord_actions_arr);
      if (stack->chord_actions_size > 0)
        {
          stack->chord_actions =
            object_new_n (stack->chord_actions_size, ChordAction *);
          yyjson_arr_iter chord_actions_it =
            yyjson_arr_iter_with (chord_actions_arr);
          yyjson_val * action_obj = NULL;
          while ((action_obj = yyjson_arr_iter_next (&chord_actions_it)))
            {
              ChordAction * action = object_new (ChordAction);
              stack->chord_actions[stack->num_chord_actions++] = action;
              chord_action_deserialize_from_json (
                doc, action_obj, action, error);
            }
        }
    }
  yyjson_val * s_obj = yyjson_obj_iter_get (&it, "stack");
  stack->stack = object_new (Stack);
  stack_deserialize_from_json (doc, s_obj, stack->stack, error);
  return true;
}

static bool
undo_manager_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  um_obj,
  UndoManager * um,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (um_obj);
  yyjson_val *    undo_stack_obj = yyjson_obj_iter_get (&it, "undoStack");
  um->undo_stack = object_new (UndoStack);
  undo_stack_deserialize_from_json (doc, undo_stack_obj, um->undo_stack, error);
  yyjson_val * redo_stack_obj = yyjson_obj_iter_get (&it, "redoStack");
  um->redo_stack = object_new (UndoStack);
  undo_stack_deserialize_from_json (doc, redo_stack_obj, um->redo_stack, error);
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

  yyjson_mut_obj_add_str (doc, root, "type", "ZrythmProject");
  yyjson_mut_obj_add_int (doc, root, "formatMajor", PROJECT_FORMAT_MAJOR);
  yyjson_mut_obj_add_int (doc, root, "formatMinor", PROJECT_FORMAT_MINOR);

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
  for (auto track : tracklist->tracks)
    {
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
  yyjson_mut_obj_add_int (
    doc, root, "lastSelection", (int64_t) prj->last_selection);

  /* to string - minified */
  char * json = yyjson_mut_write (
    doc, YYJSON_WRITE_NOFLAG /* YYJSON_WRITE_PRETTY_TWO_SPACES */, NULL);
  g_message ("done writing json to string");
  if (json)
    {
      /*g_message ("json: %s\n", json);*/
    }

  yyjson_mut_doc_free (doc);

  return json;
}

static bool
tracklist_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * tracklist_obj,
  Tracklist *  tracklist,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (tracklist_obj);
  tracklist->pinned_tracks_cutoff =
    yyjson_get_int (yyjson_obj_iter_get (&it, "pinnedTracksCutoff"));
  yyjson_val *    tracks_arr = yyjson_obj_iter_get (&it, "tracks");
  yyjson_arr_iter tracks_it = yyjson_arr_iter_with (tracks_arr);
  yyjson_val *    track_obj = NULL;
  while ((track_obj = yyjson_arr_iter_next (&tracks_it)))
    {
      Track * track = object_new (Track);
      tracklist->tracks.push_back (track);
      track_deserialize_from_json (doc, track_obj, track, error);
    }
  return true;
}

Project *
project_deserialize_from_json_str (const char * json, GError ** error)
{
  yyjson_doc * doc =
    yyjson_read_opts ((char *) json, strlen (json), 0, NULL, NULL);
  yyjson_val * root = yyjson_doc_get_root (doc);
  if (!root)
    {
      g_set_error_literal (
        error, Z_IO_SERIALIZATION_PROJECT_ERROR,
        Z_IO_SERIALIZATION_PROJECT_ERROR_FAILED, "Failed to create root obj");
      return NULL;
    }

  yyjson_obj_iter it = yyjson_obj_iter_with (root);
  if (!yyjson_equals_str (yyjson_obj_iter_get (&it, "type"), "ZrythmProject"))
    {
      g_set_error_literal (
        error, Z_IO_SERIALIZATION_PROJECT_ERROR,
        Z_IO_SERIALIZATION_PROJECT_ERROR_FAILED, "Not a Zrythm project");
      return NULL;
    }

  Project * self = new Project ();

  self->format_major = yyjson_get_int (yyjson_obj_iter_get (&it, "formatMajor"));
  self->format_minor = yyjson_get_int (yyjson_obj_iter_get (&it, "formatMinor"));
  self->title = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "title")));
  self->datetime_str =
    g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "datetime")));
  self->version =
    g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "version")));
  self->tracklist = new Tracklist ();
  tracklist_deserialize_from_json (
    doc, yyjson_obj_iter_get (&it, "tracklist"), self->tracklist, error);
  yyjson_val * clip_editor_obj = yyjson_obj_iter_get (&it, "clipEditor");
  self->clip_editor = object_new (ClipEditor);
  clip_editor_deserialize_from_json (
    doc, clip_editor_obj, self->clip_editor, error);
  yyjson_val * timeline_obj = yyjson_obj_iter_get (&it, "timeline");
  self->timeline = object_new (Timeline);
  timeline_deserialize_from_json (doc, timeline_obj, self->timeline, error);
  yyjson_val * snap_grid_timeline_obj =
    yyjson_obj_iter_get (&it, "snapGridTimeline");
  self->snap_grid_timeline = object_new (SnapGrid);
  snap_grid_deserialize_from_json (
    doc, snap_grid_timeline_obj, self->snap_grid_timeline, error);
  yyjson_val * snap_grid_editor_obj =
    yyjson_obj_iter_get (&it, "snapGridEditor");
  self->snap_grid_editor = object_new (SnapGrid);
  snap_grid_deserialize_from_json (
    doc, snap_grid_editor_obj, self->snap_grid_editor, error);
  yyjson_val * quantize_opts_timeline_obj =
    yyjson_obj_iter_get (&it, "quantizeOptsTimeline");
  self->quantize_opts_timeline = object_new (QuantizeOptions);
  quantize_options_deserialize_from_json (
    doc, quantize_opts_timeline_obj, self->quantize_opts_timeline, error);
  yyjson_val * quantize_opts_editor_obj =
    yyjson_obj_iter_get (&it, "quantizeOptsEditor");
  self->quantize_opts_editor = object_new (QuantizeOptions);
  quantize_options_deserialize_from_json (
    doc, quantize_opts_editor_obj, self->quantize_opts_editor, error);
  yyjson_val * audio_engine_obj = yyjson_obj_iter_get (&it, "audioEngine");
  self->audio_engine = object_new (AudioEngine);
  audio_engine_deserialize_from_json (
    doc, audio_engine_obj, self->audio_engine, error);
  yyjson_val * mixer_selections_obj =
    yyjson_obj_iter_get (&it, "mixerSelections");
  if (mixer_selections_obj)
    {
      self->mixer_selections = object_new (MixerSelections);
      mixer_selections_deserialize_from_json (
        doc, mixer_selections_obj, self->mixer_selections, error);
    }
  yyjson_val * timeline_selections_obj =
    yyjson_obj_iter_get (&it, "timelineSelections");
  if (timeline_selections_obj)
    {
      self->timeline_selections = object_new (TimelineSelections);
      timeline_selections_deserialize_from_json (
        doc, timeline_selections_obj, self->timeline_selections, error);
    }
  yyjson_val * midi_arranger_selections_obj =
    yyjson_obj_iter_get (&it, "midiArrangerSelections");
  if (midi_arranger_selections_obj)
    {
      self->midi_arranger_selections = object_new (MidiArrangerSelections);
      midi_arranger_selections_deserialize_from_json (
        doc, midi_arranger_selections_obj, self->midi_arranger_selections,
        error);
    }
  yyjson_val * chord_selections_obj =
    yyjson_obj_iter_get (&it, "chordSelections");
  if (chord_selections_obj)
    {
      self->chord_selections = object_new (ChordSelections);
      chord_selections_deserialize_from_json (
        doc, chord_selections_obj, self->chord_selections, error);
    }
  yyjson_val * automation_selections_obj =
    yyjson_obj_iter_get (&it, "automationSelections");
  if (automation_selections_obj)
    {
      self->automation_selections = object_new (AutomationSelections);
      automation_selections_deserialize_from_json (
        doc, automation_selections_obj, self->automation_selections, error);
    }
  yyjson_val * audio_selections_obj =
    yyjson_obj_iter_get (&it, "audioSelections");
  if (audio_selections_obj)
    {
      self->audio_selections = object_new (AudioSelections);
      audio_selections_deserialize_from_json (
        doc, audio_selections_obj, self->audio_selections, error);
    }
  yyjson_val * tracklist_selections_obj =
    yyjson_obj_iter_get (&it, "tracklistSelections");
  self->tracklist_selections = object_new (TracklistSelections);
  tracklist_selections_deserialize_from_json (
    doc, tracklist_selections_obj, self->tracklist_selections, error);
  yyjson_val * region_link_group_manager_obj =
    yyjson_obj_iter_get (&it, "regionLinkGroupManager");
  self->region_link_group_manager = object_new (RegionLinkGroupManager);
  region_link_group_manager_deserialize_from_json (
    doc, region_link_group_manager_obj, self->region_link_group_manager, error);
  yyjson_val * port_connections_manager_obj =
    yyjson_obj_iter_get (&it, "portConnectionsManager");
  self->port_connections_manager = object_new (PortConnectionsManager);
  port_connections_manager_deserialize_from_json (
    doc, port_connections_manager_obj, self->port_connections_manager, error);
  yyjson_val * midi_mappings_obj = yyjson_obj_iter_get (&it, "midiMappings");
  self->midi_mappings = object_new (MidiMappings);
  midi_mappings_deserialize_from_json (
    doc, midi_mappings_obj, self->midi_mappings, error);
  yyjson_val * undo_manager_obj = yyjson_obj_iter_get (&it, "undoManager");
  /* skip undo history from versions previous to 1.10 because of a change in how
   * stretching changes are stored */
  if (
    (self->format_major == 1 && self->format_minor >= 10)
    || self->format_major > 1)
    {
      if (undo_manager_obj)
        {
          self->undo_manager = object_new (UndoManager);
          undo_manager_deserialize_from_json (
            doc, undo_manager_obj, self->undo_manager, error);
        }
    }
  self->last_selection = (Project::SelectionType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "lastSelection"));

  yyjson_doc_free (doc);

  return self;
}
