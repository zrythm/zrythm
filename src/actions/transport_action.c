/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/transport_action.h"
#include "audio/engine.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
transport_action_init_loaded (
  TransportAction * self)
{
}

/**
 * FIXME make a general port change action.
 */
UndoableAction *
transport_action_new_bpm_change (
  bpm_t           bpm_before,
  bpm_t           bpm_after,
  bool            already_done,
  GError **       error)
{
  TransportAction * self =
    object_new (TransportAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_TRANSPORT);

  self->type = TRANSPORT_ACTION_BPM_CHANGE;

  self->bpm_before = bpm_before;
  self->bpm_after = bpm_after;
  self->already_done = already_done;
  self->musical_mode =
    g_settings_get_boolean (S_UI, "musical-mode");

  return ua;
}

UndoableAction *
transport_action_new_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error)
{
  TransportAction * self =
    object_new (TransportAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_TRANSPORT);

  self->type = type;

  self->int_before = before;
  self->int_after = after;
  self->already_done = already_done;
  self->musical_mode =
    g_settings_get_boolean (S_UI, "musical-mode");

  return ua;
}

TransportAction *
transport_action_clone (
  const TransportAction * src)
{
  TransportAction * self =
    object_new (TransportAction);
  self->parent_instance = src->parent_instance;

  self->type = src->type;
  self->bpm_before = src->bpm_before;
  self->bpm_after = src->bpm_after;
  self->int_before = src->int_before;
  self->int_after = src->int_after;
  self->musical_mode = src->musical_mode;

  return self;
}

bool
transport_action_perform_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    transport_action_new_bpm_change,
    error, bpm_before, bpm_after, already_done,
    error);
}

bool
transport_action_perform_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    transport_action_new_time_sig_change,
    error, type, before, after, already_done,
    error);
}

static int
do_or_undo (
  TransportAction * self,
  bool              _do)
{
  switch (self->type)
    {
    case TRANSPORT_ACTION_BPM_CHANGE:
      port_set_control_value (
        P_TEMPO_TRACK->bpm_port,
        _do ? self->bpm_after : self->bpm_before,
        false, false);
      g_message ("set BPM to %f",
        (double)
        tempo_track_get_current_bpm (P_TEMPO_TRACK));
      break;
    case TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      tempo_track_set_beats_per_bar (
        P_TEMPO_TRACK,
        _do ? self->int_after : self->int_before);
      break;
    case TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      tempo_track_set_beat_unit (
        P_TEMPO_TRACK,
        _do ? self->int_after : self->int_before);
      break;
    }

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar,
    tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate, true);

  snap_grid_update_snap_points_default (
    SNAP_GRID_TIMELINE);
  snap_grid_update_snap_points_default (
    SNAP_GRID_EDITOR);

  if (self->type == TRANSPORT_ACTION_BPM_CHANGE)
    {
      /* get time ratio */
      double time_ratio = 0;
      if (_do)
        {
          time_ratio =
            self->bpm_after / self->bpm_before;
        }
      else
        {
          time_ratio =
            self->bpm_before / self->bpm_after;
        }

      if (self->musical_mode &&
          /* doesn't work */
          false)
        {
          transport_stretch_audio_regions (
            TRANSPORT, NULL, true, time_ratio);
        }
    }

  return 0;
}

int
transport_action_do (
  TransportAction * self,
  GError **         error)
{
  if (self->already_done)
    {
      self->already_done = false;

      int beats_per_bar =
        tempo_track_get_beats_per_bar (
          P_TEMPO_TRACK);
      bpm_t bpm =
        tempo_track_get_current_bpm (P_TEMPO_TRACK);

      engine_update_frames_per_tick (
        AUDIO_ENGINE, beats_per_bar, bpm,
        AUDIO_ENGINE->sample_rate, true);

      snap_grid_update_snap_points_default (
        SNAP_GRID_TIMELINE);
      snap_grid_update_snap_points_default (
        SNAP_GRID_EDITOR);
    }
  else
    {
      do_or_undo (self, true);
    }

  EVENTS_PUSH (ET_BPM_CHANGED, NULL);
  EVENTS_PUSH (ET_TIME_SIGNATURE_CHANGED, NULL);

  return 0;
}

int
transport_action_undo (
  TransportAction * self,
  GError **         error)
{
  do_or_undo (self, false);

  EVENTS_PUSH (ET_BPM_CHANGED, NULL);
  EVENTS_PUSH (ET_TIME_SIGNATURE_CHANGED, NULL);

  return 0;
}

char *
transport_action_stringize (
  TransportAction * self)
{
  switch (self->type)
    {
    case TRANSPORT_ACTION_BPM_CHANGE:
      return g_strdup (_("Change BPM"));
    case TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      return g_strdup (_("Beats per bar change"));
    case TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      return g_strdup (_("Beat unit change"));
    }

  g_return_val_if_reached (NULL);
}

void
transport_action_free (
  TransportAction * self)
{
  free (self);
}
