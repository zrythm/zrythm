/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "actions/arranger_selections.h"
#include "actions/mixer_selections_action.h"
#include "actions/range_action.h"
#include "actions/tracklist_selections.h"
#include "actions/transport_action.h"
#include "actions/undoable_action.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm_app.h"

#include <glib.h>
#include <glib/gi18n.h>

void
undoable_action_init_loaded (
  UndoableAction * self)
{
  /* uppercase, camel case, snake case */
#define INIT_LOADED(uc,sc,cc) \
  case UA_##uc: \
    sc##_action_init_loaded ((cc##Action *) self); \
    break;

  switch (self->type)
    {
    INIT_LOADED (
      TRACKLIST_SELECTIONS,
      tracklist_selections,
      TracklistSelections);
    INIT_LOADED (CHANNEL_SEND,
               channel_send,
               ChannelSend);
    INIT_LOADED (
      MIXER_SELECTIONS, mixer_selections,
      MixerSelections);
    INIT_LOADED (
      ARRANGER_SELECTIONS, arranger_selections,
      ArrangerSelections);
    INIT_LOADED (
      MIDI_MAPPING, midi_mapping, MidiMapping);
    INIT_LOADED (
      PORT_CONNECTION, port_connection,
      PortConnection);
    INIT_LOADED (PORT, port, Port);
    INIT_LOADED (TRANSPORT, transport, Transport);
    INIT_LOADED (RANGE, range, Range);
    default:
      break;
    }

#undef INIT_LOADED
}

/**
 * Initializer to be used by implementing actions.
 */
void
undoable_action_init (
  UndoableAction *   self,
  UndoableActionType type)
{
  self->schema_version =
    UNDOABLE_ACTION_SCHEMA_VERSION;
  self->type = type;
  self->num_actions = 1;
}

/**
 * Returns whether the total transport bars need to
 * be recalculated.
 *
 * @note Some actions already handle this logic so
 *   return false here to avoid unnecessary
 *   calculations.
 */
static bool
need_transport_total_bar_update (
  UndoableAction * self,
  bool             _do)
{
  switch (self->type)
    {
    case UA_ARRANGER_SELECTIONS:
      {
        ArrangerSelectionsAction * action =
          (ArrangerSelectionsAction *) self;
        if ((action->type == AS_ACTION_CREATE &&
              _do) ||
            (action->type == AS_ACTION_DELETE &&
              !_do) ||
            (action->type == AS_ACTION_DUPLICATE &&
              _do) ||
            (action->type == AS_ACTION_LINK &&
              _do))
          {
            return false;
          }
      }
      break;
    case UA_TRACKLIST_SELECTIONS:
      {
        TracklistSelectionsAction * action =
          (TracklistSelectionsAction *) self;
        if (action->type ==
              TRACKLIST_SELECTIONS_ACTION_EDIT)
          {
            if (action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_MUTE ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_SOLO ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_LISTEN ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_VOLUME ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_PAN)
              {
                return false;
              }
          }
      }
      break;
    default:
      break;
    }

  return true;
}

/**
 * Returns whether the action requires pausing
 * the engine.
 */
bool
undoable_action_needs_pause (
  UndoableAction * self)
{
  switch (self->type)
    {
    case UA_ARRANGER_SELECTIONS:
      {
        ArrangerSelectionsAction * action =
          (ArrangerSelectionsAction *) self;
        if (action->type == AS_ACTION_EDIT)
          {
            if (action->edit_type ==
                  ARRANGER_SELECTIONS_ACTION_EDIT_MUTE)
              {
                return false;
              }
          }
      }
      break;
    case UA_TRACKLIST_SELECTIONS:
      {
        TracklistSelectionsAction * action =
          (TracklistSelectionsAction *) self;
        if (action->type ==
              TRACKLIST_SELECTIONS_ACTION_EDIT)
          {
            if (action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_MUTE ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_SOLO ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_LISTEN ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_VOLUME ||
                action->edit_type ==
                  EDIT_TRACK_ACTION_TYPE_PAN)
              {
                return false;
              }
          }
      }
      break;
    default:
      break;
    }

  return true;
}

/**
 * Checks whether the action can contain an audio
 * clip.
 *
 * No attempt is made to remove unused files from
 * the pool for actions that can't contain audio
 * clips.
 */
bool
undoable_action_can_contain_clip (
  UndoableAction * self)
{
  switch (self->type)
    {
    case UA_TRACKLIST_SELECTIONS:
      {
        TracklistSelectionsAction * action =
          (TracklistSelectionsAction *) self;
        return action->pool_id >= 0;
      }
      break;
    case UA_ARRANGER_SELECTIONS:
    case UA_RANGE:
      return true;
      break;
    default:
      break;
    }

  return false;
}

/**
 * Performs the action.
 *
 * @note Only to be called by undo manager.
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_do (
  UndoableAction * self,
  GError **        error)
{
#if 0
  g_debug ("waiting for port operation lock...");
  zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);
  g_debug ("lock acquired");
#endif

  EngineState state;
  if (undoable_action_needs_pause (self))
    {
      /* stop engine and give it some time to stop
       * running */
      engine_wait_for_pause (
        AUDIO_ENGINE, &state, F_NO_FORCE);
    }

  int ret = 0;

  /* uppercase, camel case, snake case */
#define DO_ACTION(uc,sc,cc) \
  case UA_##uc: \
    { \
      char * str = \
        undoable_action_to_string (self); \
      g_message ( \
        "[DOING ACTION]: " #uc " (%s)", str); \
      ret = \
        sc##_action_do ( \
          (cc##Action *) self, error); \
      if (ret == 0) \
        { \
          g_message ("[DONE]: " #uc " (%s)", str); \
        } \
      else \
        { \
          g_warning ( \
            "[FAILED]: " #uc " (%s)", str); \
        } \
      g_free (str); \
    } \
    break;

  switch (self->type)
    {
    DO_ACTION (
      TRACKLIST_SELECTIONS,
      tracklist_selections,
      TracklistSelections);
    DO_ACTION (CHANNEL_SEND,
               channel_send,
               ChannelSend);
    DO_ACTION (
      MIXER_SELECTIONS, mixer_selections,
      MixerSelections);
    DO_ACTION (
      ARRANGER_SELECTIONS, arranger_selections,
      ArrangerSelections);
    DO_ACTION (TRANSPORT, transport, Transport);
    DO_ACTION (RANGE, range, Range);
    DO_ACTION (
      PORT_CONNECTION, port_connection,
      PortConnection);
    DO_ACTION (PORT, port, Port);
    DO_ACTION (
      MIDI_MAPPING, midi_mapping, MidiMapping);
    default:
      g_warn_if_reached ();
      ret = -1;
    }

#undef DO_ACTION

#if 0
  g_debug ("releasing lock...");
  zix_sem_post (&AUDIO_ENGINE->port_operation_lock);
  g_debug ("lock released");
#endif

  if (need_transport_total_bar_update (self, true))
    {
      /* recalculate transport bars */
      transport_recalculate_total_bars (
        TRANSPORT, NULL);
    }

  if (undoable_action_needs_pause (self))
    {
      /* restart engine */
      engine_resume (AUDIO_ENGINE, &state);
    }

  return ret;
}

/**
 * Undoes the action.
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_undo (
  UndoableAction * self,
  GError **        error)
{
  /*zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);*/

  EngineState state;
  if (undoable_action_needs_pause (self))
    {
      /* stop engine and give it some time to stop
       * running */
      engine_wait_for_pause (
        AUDIO_ENGINE, &state, F_NO_FORCE);
    }

  int ret = 0;

/* uppercase, camel case, snake case */
#define UNDO_ACTION(uc,sc,cc) \
  case UA_##uc: \
    { \
      char * str = \
        undoable_action_to_string (self); \
      g_message ( \
        "[UNDOING ACTION]: " #uc " (%s)", str); \
      ret = \
        sc##_action_undo ( \
          (cc##Action *) self, error); \
      if (ret == 0) \
        { \
          g_message ("[UNDONE]: " #uc " (%s)", str); \
        } \
      else \
        { \
          g_warning ( \
            "[FAILED]: " #uc " (%s)", str); \
        } \
      g_free (str); \
    } \
    break;

  switch (self->type)
    {
    UNDO_ACTION (
      TRACKLIST_SELECTIONS, tracklist_selections,
      TracklistSelections);
    UNDO_ACTION (
      CHANNEL_SEND, channel_send, ChannelSend);
    UNDO_ACTION (
      MIXER_SELECTIONS, mixer_selections,
      MixerSelections);
    UNDO_ACTION (
      ARRANGER_SELECTIONS, arranger_selections,
      ArrangerSelections);
    UNDO_ACTION (
      MIDI_MAPPING, midi_mapping, MidiMapping);
    UNDO_ACTION (
      PORT_CONNECTION, port_connection,
      PortConnection);
    UNDO_ACTION (PORT, port, Port);
    UNDO_ACTION (TRANSPORT, transport, Transport);
    UNDO_ACTION (RANGE, range, Range);
    default:
      g_warn_if_reached ();
      ret = -1;
    }

#undef UNDO_ACTION

  /*zix_sem_post (&AUDIO_ENGINE->port_operation_lock);*/

  if (need_transport_total_bar_update (self, false))
    {
      /* recalculate transport bars */
      transport_recalculate_total_bars (
        TRANSPORT, NULL);
    }

  if (undoable_action_needs_pause (self))
    {
      /* restart engine */
      engine_resume (AUDIO_ENGINE, &state);
    }

  return ret;
}

/**
 * Checks whether the action actually contains or
 * refers to the given audio clip.
 */
bool
undoable_action_contains_clip (
  UndoableAction * self,
  AudioClip *      clip)
{
  bool ret = false;

  switch (self->type)
    {
    case UA_TRACKLIST_SELECTIONS:
      {
        TracklistSelectionsAction * action =
          (TracklistSelectionsAction *) self;
        ret = clip->pool_id == action->pool_id;
      }
      break;
    case UA_ARRANGER_SELECTIONS:
      {
        ArrangerSelectionsAction * action =
          (ArrangerSelectionsAction *) self;
        ret =
          arranger_selections_action_contains_clip (
            action, clip);
      }
      break;
    case UA_RANGE:
      {
        RangeAction * action = (RangeAction *) self;
        ret =
          (action->sel_before &&
           arranger_selections_contains_clip (
             (ArrangerSelections *)
             action->sel_before, clip)) ||
          (action->sel_after &&
           arranger_selections_contains_clip (
             (ArrangerSelections *)
             action->sel_after, clip));
      }
      break;
    default:
      break;
    }

  if (ret)
    {
      char * str = undoable_action_to_string (self);
      g_debug (
        "undoable action %s contains clip %s",
        str, clip->name);
      g_free (str);
    }

  return ret;
}

/**
 * Sets the number of actions for this action.
 *
 * This should be set on the last action to be
 * performed.
 */
void
undoable_action_set_num_actions (
  UndoableAction * self,
  int              num_actions)
{
  g_return_if_fail (
    num_actions > 0 &&
    num_actions < ZRYTHM->undo_stack_len);
  self->num_actions = num_actions;
}

/**
 * To be used by actions that save/load port
 * connections.
 *
 * @param _do True if doing/performing, false if
 *   undoing.
 * @param before Pointer to the connections before.
 * @param after Pointer to the connections after.
 */
void
undoable_action_save_or_load_port_connections (
  UndoableAction *          self,
  bool                      _do,
  PortConnectionsManager ** before,
  PortConnectionsManager ** after)
{
  /* if first do and keeping track of connections,
   * clone the new connections */
  if (_do && *before != NULL && *after == NULL)
    {
      g_debug (
        "updating and caching port connections "
        "after doing action");
      *after =
        port_connections_manager_clone (
          PORT_CONNECTIONS_MGR);
    }
  else if (_do && *after != NULL)
    {
      g_debug (
        "resetting port connections from "
        "cached after");
      port_connections_manager_reset (
        PORT_CONNECTIONS_MGR, *after);
      g_return_if_fail (
        PORT_CONNECTIONS_MGR->num_connections ==
          (*after)->num_connections);
    }
  /* else if undoing and have connections from
   * before */
  else if (!_do && *before != NULL)
    {
      /* reset the connections */
      g_debug (
        "resetting port connections from "
        "cached before");
      port_connections_manager_reset (
        PORT_CONNECTIONS_MGR, *before);
      g_return_if_fail (
        PORT_CONNECTIONS_MGR->num_connections ==
          (*before)->num_connections);
    }
}

/**
 * Stringizes the action to be used in Undo/Redo
 * buttons.
 *
 * The string MUST be free'd using g_free().
 */
char *
undoable_action_to_string (
  UndoableAction * ua)
{
#define STRINGIZE_UA(caps,cc,sc) \
  case UA_##caps: \
    return sc##_action_stringize ( \
      (cc##Action *) ua);

  switch (ua->type)
    {
    STRINGIZE_UA (
      TRACKLIST_SELECTIONS,
      TracklistSelections,
      tracklist_selections);
    STRINGIZE_UA (CHANNEL_SEND,
               ChannelSend,
               channel_send);
    STRINGIZE_UA (
      MIXER_SELECTIONS, MixerSelections,
      mixer_selections);
    STRINGIZE_UA (
      ARRANGER_SELECTIONS, ArrangerSelections,
      arranger_selections);
    STRINGIZE_UA (
      MIDI_MAPPING, MidiMapping, midi_mapping);
    STRINGIZE_UA (
      PORT_CONNECTION, PortConnection,
      port_connection);
    STRINGIZE_UA (PORT, Port, port);
    STRINGIZE_UA (TRANSPORT, Transport, transport);
    STRINGIZE_UA (RANGE, Range, range);
    default:
      g_return_val_if_reached (
        g_strdup (""));
    }

#undef STRINGIZE_UA
}

void
undoable_action_free (UndoableAction * self)
{
/* uppercase, camel case, snake case */
#define FREE_ACTION(uc,sc,cc) \
  case UA_##uc: \
    sc##_action_free ((cc##Action *) self); \
    break;

  switch (self->type)
    {
    FREE_ACTION (
      TRACKLIST_SELECTIONS,
      tracklist_selections,
      TracklistSelections);
    FREE_ACTION (CHANNEL_SEND,
               channel_send,
               ChannelSend);
    FREE_ACTION (
      MIXER_SELECTIONS, mixer_selections,
      MixerSelections);
    FREE_ACTION (
      ARRANGER_SELECTIONS, arranger_selections,
      ArrangerSelections);
    FREE_ACTION (
      MIDI_MAPPING, midi_mapping, MidiMapping);
    FREE_ACTION (
      PORT_CONNECTION, port_connection,
      PortConnection);
    FREE_ACTION (PORT, port, Port);
    FREE_ACTION (TRANSPORT, transport, Transport);
    FREE_ACTION (RANGE, range, Range);
    default:
      g_warn_if_reached ();
      break;
    }

#undef FREE_ACTION
}
