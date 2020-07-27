/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "actions/copy_plugins_action.h"
#include "actions/copy_tracks_action.h"
#include "actions/create_plugins_action.h"
#include "actions/create_tracks_action.h"
#include "actions/delete_plugins_action.h"
#include "actions/delete_tracks_action.h"
#include "actions/edit_plugins_action.h"
#include "actions/edit_tracks_action.h"
#include "actions/move_plugins_action.h"
#include "actions/move_tracks_action.h"
#include "actions/undoable_action.h"
#include "actions/transport_action.h"
#include "project.h"
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
    /*INIT_LOADED (CREATE_TRACKS,*/
               /*create_tracks,*/
               /*CreateTracks);*/
    INIT_LOADED (MOVE_TRACKS,
               move_tracks,
               MoveTracks);
    INIT_LOADED (EDIT_TRACKS,
               edit_tracks,
               EditTracks);
    INIT_LOADED (COPY_TRACKS,
               copy_tracks,
               CopyTracks);
    INIT_LOADED (DELETE_TRACKS,
               delete_tracks,
               DeleteTracks);
    INIT_LOADED (CHANNEL_SEND,
               channel_send,
               ChannelSend);
    /*INIT_LOADED (CREATE_PLUGINS,*/
               /*create_plugins,*/
               /*CreatePlugins);*/
    INIT_LOADED (MOVE_PLUGINS,
               move_plugins,
               MovePlugins);
    INIT_LOADED (EDIT_PLUGINS,
               edit_plugins,
               EditPlugins);
    INIT_LOADED (COPY_PLUGINS,
               copy_plugins,
               CopyPlugins);
    INIT_LOADED (DELETE_PLUGINS,
               delete_plugins,
               DeletePlugins);
    INIT_LOADED (CREATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (MOVE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (LINK_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (RECORD_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (EDIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (RESIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (DUPLICATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (DELETE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (QUANTIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (SPLIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    INIT_LOADED (
      PORT_CONNECTION, port_connection,
      PortConnection);
    /*INIT_LOADED (TRANSPORT, transport, Transport);*/
    default:
      break;
    }

#undef INIT_LOADED
}

/**
 * Performs the action.
 *
 * @note Only to be called by undo manager.
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_do (UndoableAction * self)
{
  zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);

  int ret = 0;

  /* uppercase, camel case, snake case */
#define DO_ACTION(uc,sc,cc) \
  case UA_##uc: \
    { \
      char * str = \
        undoable_action_stringize (self); \
      g_message ( \
        "[DOING ACTION]: " #uc " (%s)", str); \
      g_free (str); \
      ret = sc##_action_do ((cc##Action *) self); \
      g_message ("[DONE]: " #uc); \
    } \
    break;

  switch (self->type)
    {
    DO_ACTION (CREATE_TRACKS,
               create_tracks,
               CreateTracks);
    DO_ACTION (MOVE_TRACKS,
               move_tracks,
               MoveTracks);
    DO_ACTION (EDIT_TRACKS,
               edit_tracks,
               EditTracks);
    DO_ACTION (COPY_TRACKS,
               copy_tracks,
               CopyTracks);
    DO_ACTION (DELETE_TRACKS,
               delete_tracks,
               DeleteTracks);
    DO_ACTION (CHANNEL_SEND,
               channel_send,
               ChannelSend);
    DO_ACTION (CREATE_PLUGINS,
               create_plugins,
               CreatePlugins);
    DO_ACTION (MOVE_PLUGINS,
               move_plugins,
               MovePlugins);
    DO_ACTION (EDIT_PLUGINS,
               edit_plugins,
               EditPlugins);
    DO_ACTION (COPY_PLUGINS,
               copy_plugins,
               CopyPlugins);
    DO_ACTION (DELETE_PLUGINS,
               delete_plugins,
               DeletePlugins);
    DO_ACTION (CREATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (MOVE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (LINK_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (RECORD_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (EDIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (RESIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (DUPLICATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (DELETE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (QUANTIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (SPLIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    DO_ACTION (TRANSPORT, transport, Transport);
    DO_ACTION (
      PORT_CONNECTION, port_connection,
      PortConnection);
    default:
      g_warn_if_reached ();
      ret = -1;
    }

#undef DO_ACTION

  zix_sem_post (&AUDIO_ENGINE->port_operation_lock);

  return ret;
}

/**
 * Undoes the action.
 *
 * Note: only to be called by undo manager.
 */
int
undoable_action_undo (UndoableAction * self)
{
  zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);

  int ret = 0;

/* uppercase, camel case, snake case */
#define UNDO_ACTION(uc,sc,cc) \
  case UA_##uc: \
    { \
      char * str = \
        undoable_action_stringize (self); \
      g_message ( \
        "[UNDOING ACTION]: " #uc " (%s)", str); \
      g_free (str); \
      ret = \
        sc##_action_undo ((cc##Action *) self); \
      g_message ("[UNDONE]: " #uc); \
    } \
    break;

  switch (self->type)
    {
    UNDO_ACTION (CREATE_TRACKS,
               create_tracks,
               CreateTracks);
    UNDO_ACTION (MOVE_TRACKS,
               move_tracks,
               MoveTracks);
    UNDO_ACTION (EDIT_TRACKS,
               edit_tracks,
               EditTracks);
    UNDO_ACTION (COPY_TRACKS,
               copy_tracks,
               CopyTracks);
    UNDO_ACTION (DELETE_TRACKS,
               delete_tracks,
               DeleteTracks);
    UNDO_ACTION (CHANNEL_SEND,
               channel_send,
               ChannelSend);
    UNDO_ACTION (CREATE_PLUGINS,
               create_plugins,
               CreatePlugins);
    UNDO_ACTION (MOVE_PLUGINS,
               move_plugins,
               MovePlugins);
    UNDO_ACTION (EDIT_PLUGINS,
               edit_plugins,
               EditPlugins);
    UNDO_ACTION (COPY_PLUGINS,
               copy_plugins,
               CopyPlugins);
    UNDO_ACTION (DELETE_PLUGINS,
               delete_plugins,
               DeletePlugins);
    UNDO_ACTION (CREATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (MOVE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (LINK_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (RECORD_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (EDIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (RESIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (DUPLICATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (DELETE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (QUANTIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (SPLIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    UNDO_ACTION (
      PORT_CONNECTION, port_connection,
      PortConnection);
    UNDO_ACTION (TRANSPORT, transport, Transport);
    default:
      g_warn_if_reached ();
      ret = -1;
    }

#undef UNDO_ACTION

  zix_sem_post (&AUDIO_ENGINE->port_operation_lock);

  return ret;
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
    FREE_ACTION (CREATE_TRACKS,
               create_tracks,
               CreateTracks);
    FREE_ACTION (MOVE_TRACKS,
               move_tracks,
               MoveTracks);
    FREE_ACTION (EDIT_TRACKS,
               edit_tracks,
               EditTracks);
    FREE_ACTION (COPY_TRACKS,
               copy_tracks,
               CopyTracks);
    FREE_ACTION (DELETE_TRACKS,
               delete_tracks,
               DeleteTracks);
    FREE_ACTION (CHANNEL_SEND,
               channel_send,
               ChannelSend);
    FREE_ACTION (CREATE_PLUGINS,
               create_plugins,
               CreatePlugins);
    FREE_ACTION (MOVE_PLUGINS,
               move_plugins,
               MovePlugins);
    FREE_ACTION (EDIT_PLUGINS,
               edit_plugins,
               EditPlugins);
    FREE_ACTION (COPY_PLUGINS,
               copy_plugins,
               CopyPlugins);
    FREE_ACTION (DELETE_PLUGINS,
               delete_plugins,
               DeletePlugins);
    FREE_ACTION (CREATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (MOVE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (LINK_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (EDIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (RESIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (DUPLICATE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (DELETE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (QUANTIZE_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (SPLIT_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (RECORD_ARRANGER_SELECTIONS,
               arranger_selections,
               ArrangerSelections);
    FREE_ACTION (
      PORT_CONNECTION, port_connection,
      PortConnection);
    FREE_ACTION (TRANSPORT, transport, Transport);
    default:
      g_warn_if_reached ();
      break;
    }

#undef FREE_ACTION
}

/**
 * Stringizes the action to be used in Undo/Redo
 * buttons.
 *
 * The string MUST be free'd using g_free().
 */
char *
undoable_action_stringize (
  UndoableAction * ua)
{
#define STRINGIZE_UA(caps,cc,sc) \
  case UA_##caps: \
    return sc##_action_stringize ( \
      (cc##Action *) ua);

  switch (ua->type)
    {
    STRINGIZE_UA (CREATE_TRACKS,
                  CreateTracks,
                  create_tracks);
    STRINGIZE_UA (MOVE_TRACKS,
                  MoveTracks,
                  move_tracks);
    STRINGIZE_UA (EDIT_TRACKS,
                  EditTracks,
                  edit_tracks);
    STRINGIZE_UA (COPY_TRACKS,
                  CopyTracks,
                  copy_tracks);
    STRINGIZE_UA (DELETE_TRACKS,
                  DeleteTracks,
                  delete_tracks);
    STRINGIZE_UA (CHANNEL_SEND,
               ChannelSend,
               channel_send);
    STRINGIZE_UA (CREATE_PLUGINS,
                  CreatePlugins,
                  create_plugins);
    STRINGIZE_UA (MOVE_PLUGINS,
                  MovePlugins,
                  move_plugins);
    STRINGIZE_UA (EDIT_PLUGINS,
                  EditPlugins,
                  edit_plugins);
    STRINGIZE_UA (COPY_PLUGINS,
                  CopyPlugins,
                  copy_plugins);
    STRINGIZE_UA (DELETE_PLUGINS,
                  DeletePlugins,
                  delete_plugins);
    STRINGIZE_UA (CREATE_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (MOVE_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (LINK_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (RECORD_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (EDIT_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (RESIZE_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (DUPLICATE_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (DELETE_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (QUANTIZE_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (SPLIT_ARRANGER_SELECTIONS,
               ArrangerSelections,
               arranger_selections);
    STRINGIZE_UA (
      PORT_CONNECTION, PortConnection, port_connection);
    STRINGIZE_UA (
      TRANSPORT, Transport, transport);
    default:
      g_return_val_if_reached (
        g_strdup (""));
    }

#undef STRINGIZE_UA
}
