// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/midi_mapping_action.h"
#include "dsp/channel.h"
#include "dsp/port_identifier.h"
#include "dsp/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
midi_mapping_action_init_loaded (MidiMappingAction * self)
{
}

/**
 * Creates a new action.
 */
UndoableAction *
midi_mapping_action_new_enable (int idx, bool enable, GError ** error)
{
  MidiMappingAction * self = object_new (MidiMappingAction);
  UndoableAction *    ua = (UndoableAction *) self;
  undoable_action_init (ua, UndoableActionType::UA_MIDI_MAPPING);

  self->type =
    enable
      ? MidiMappingActionType::MIDI_MAPPING_ACTION_ENABLE
      : MidiMappingActionType::MIDI_MAPPING_ACTION_DISABLE;
  self->idx = idx;

  return ua;
}

/**
 * Creates a new action.
 */
UndoableAction *
midi_mapping_action_new_bind (
  midi_byte_t * buf,
  ExtPort *     device_port,
  Port *        dest_port,
  GError **     error)
{
  MidiMappingAction * self = object_new (MidiMappingAction);
  UndoableAction *    ua = (UndoableAction *) self;
  undoable_action_init (ua, UndoableActionType::UA_MIDI_MAPPING);

  self->type = MidiMappingActionType::MIDI_MAPPING_ACTION_BIND;
  memcpy (self->buf, buf, 3 * sizeof (midi_byte_t));
  if (device_port)
    {
      self->dev_port = ext_port_clone (device_port);
    }
  self->dest_port_id = new PortIdentifier (dest_port->id);

  return ua;
}

/**
 * Creates a new action.
 */
UndoableAction *
midi_mapping_action_new_unbind (int idx, GError ** error)
{
  MidiMappingAction * self = object_new (MidiMappingAction);
  UndoableAction *    ua = (UndoableAction *) self;
  undoable_action_init (ua, UndoableActionType::UA_MIDI_MAPPING);

  self->type = MidiMappingActionType::MIDI_MAPPING_ACTION_UNBIND;
  self->idx = idx;

  return ua;
}

MidiMappingAction *
midi_mapping_action_clone (const MidiMappingAction * src)
{
  MidiMappingAction * self = object_new (MidiMappingAction);
  self->parent_instance = src->parent_instance;

  self->idx = src->idx;
  if (src->dest_port_id)
    {
      self->dest_port_id = new PortIdentifier (*src->dest_port_id);
    }
  if (src->dev_port)
    {
      self->dev_port = ext_port_clone (src->dev_port);
    }
  memcpy (self->buf, src->buf, 3 * sizeof (midi_byte_t));

  return self;
}

/**
 * Wrapper of midi_mapping_action_new_enable().
 */
bool
midi_mapping_action_perform_enable (int idx, bool enable, GError ** error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    midi_mapping_action_new_enable, error, idx, enable, error);
}

/**
 * Wrapper of midi_mapping_action_new_bind().
 */
bool
midi_mapping_action_perform_bind (
  midi_byte_t * buf,
  ExtPort *     device_port,
  Port *        dest_port,
  GError **     error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    midi_mapping_action_new_bind, error, buf, device_port, dest_port, error);
}

/**
 * Wrapper of midi_mapping_action_new_unbind().
 */
bool
midi_mapping_action_perform_unbind (int idx, GError ** error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    midi_mapping_action_new_unbind, error, idx, error);
}

static void
bind_or_unbind (MidiMappingAction * self, bool bind)
{
  if (bind)
    {
      Port * port = port_find_from_identifier (self->dest_port_id);
      self->idx = MIDI_MAPPINGS->num_mappings;
      midi_mappings_bind_device (
        MIDI_MAPPINGS, self->buf, self->dev_port, port, F_NO_PUBLISH_EVENTS);
    }
  else
    {
      MidiMapping * mapping = MIDI_MAPPINGS->mappings[self->idx];
      memcpy (self->buf, mapping->key, 3 * sizeof (midi_byte_t));
      if (self->dev_port)
        {
          ext_port_free (self->dev_port);
        }
      if (mapping->device_port)
        {
          self->dev_port = ext_port_clone (mapping->device_port);
        }
      object_delete_and_null (self->dest_port_id);
      self->dest_port_id = new PortIdentifier (mapping->dest_id);
      midi_mappings_unbind (MIDI_MAPPINGS, self->idx, F_NO_PUBLISH_EVENTS);
    }
}

int
midi_mapping_action_do (MidiMappingAction * self, GError ** error)
{
  switch (self->type)
    {
    case MidiMappingActionType::MIDI_MAPPING_ACTION_ENABLE:
      midi_mapping_set_enabled (MIDI_MAPPINGS->mappings[self->idx], true);
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_DISABLE:
      midi_mapping_set_enabled (MIDI_MAPPINGS->mappings[self->idx], false);
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_BIND:
      bind_or_unbind (self, true);
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_UNBIND:
      bind_or_unbind (self, false);
      break;
    default:
      break;
    }

  EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, NULL);

  return 0;
}

/**
 * Edits the plugin.
 */
int
midi_mapping_action_undo (MidiMappingAction * self, GError ** error)
{
  switch (self->type)
    {
    case MidiMappingActionType::MIDI_MAPPING_ACTION_ENABLE:
      midi_mapping_set_enabled (MIDI_MAPPINGS->mappings[self->idx], false);
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_DISABLE:
      midi_mapping_set_enabled (MIDI_MAPPINGS->mappings[self->idx], true);
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_BIND:
      bind_or_unbind (self, false);
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_UNBIND:
      bind_or_unbind (self, true);
      break;
    default:
      break;
    }

  EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, NULL);

  return 0;
}

char *
midi_mapping_action_stringize (MidiMappingAction * self)
{
  switch (self->type)
    {
    case MidiMappingActionType::MIDI_MAPPING_ACTION_ENABLE:
      return g_strdup (_ ("MIDI mapping enable"));
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_DISABLE:
      return g_strdup (_ ("MIDI mapping disable"));
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_BIND:
      return g_strdup (_ ("MIDI mapping bind"));
      break;
    case MidiMappingActionType::MIDI_MAPPING_ACTION_UNBIND:
      return g_strdup (_ ("MIDI mapping unbind"));
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  g_return_val_if_reached (NULL);
}

void
midi_mapping_action_free (MidiMappingAction * self)
{
  object_free_w_func_and_null (ext_port_free, self->dev_port);

  object_delete_and_null (self->dest_port_id);

  object_zero_and_free (self);
}
