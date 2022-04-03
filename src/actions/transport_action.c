/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/control_port.h"
#include "audio/engine.h"
#include "audio/router.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
transport_action_init_loaded (TransportAction * self)
{
}

/**
 * FIXME make a general port change action.
 */
UndoableAction *
transport_action_new_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error)
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
    ZRYTHM_TESTING
      ? false
      : g_settings_get_boolean (S_UI, "musical-mode");

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
    ZRYTHM_TESTING
      ? false
      : g_settings_get_boolean (S_UI, "musical-mode");

  return ua;
}

TransportAction *
transport_action_clone (const TransportAction * src)
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
    transport_action_new_bpm_change, error,
    bpm_before, bpm_after, already_done, error);
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
    transport_action_new_time_sig_change, error,
    type, before, after, already_done, error);
}

static bool
need_update_positions_from_ticks (
  TransportAction * self)
{
  switch (self->type)
    {
    case TRANSPORT_ACTION_BPM_CHANGE:
      return true;
    case TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      return true;
    case TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      return false;
    }

  g_return_val_if_reached (false);
}

static int
do_or_undo (TransportAction * self, bool _do)
{
  ControlPortChange change = { 0 };
  switch (self->type)
    {
    case TRANSPORT_ACTION_BPM_CHANGE:
      change.flag1 = PORT_FLAG_BPM;
      change.real_val =
        _do ? self->bpm_after : self->bpm_before;
      break;
    case TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      change.flag2 = PORT_FLAG2_BEATS_PER_BAR;
      change.ival =
        _do ? self->int_after : self->int_before;
      break;
    case TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      change.flag2 = change.flag2 =
        PORT_FLAG2_BEAT_UNIT;
      change
        .beat_unit = tempo_track_beat_unit_to_enum (
        _do ? self->int_after : self->int_before);
      break;
    }

  /* queue change */
  router_queue_control_port_change (ROUTER, &change);

  /* run engine to apply the change */
  engine_process_prepare (AUDIO_ENGINE, 1);
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame =
      (unsigned_frame_t) PLAYHEAD->frames,
    .local_offset = 0,
    .nframes = 1,
  };
  router_start_cycle (ROUTER, time_nfo);
  engine_post_process (AUDIO_ENGINE, 0, 1);

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  bool update_from_ticks =
    need_update_positions_from_ticks (self);
  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar,
    tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate, true,
    update_from_ticks, false);

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

      if (
        self->musical_mode &&
        /* doesn't work */
        false)
        {
          transport_stretch_regions (
            TRANSPORT, NULL, true, time_ratio,
            Z_F_NO_FORCE);
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
        tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      bpm_t bpm =
        tempo_track_get_current_bpm (P_TEMPO_TRACK);

      bool update_from_ticks =
        need_update_positions_from_ticks (self);
      engine_update_frames_per_tick (
        AUDIO_ENGINE, beats_per_bar, bpm,
        AUDIO_ENGINE->sample_rate, true,
        update_from_ticks, false);
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
transport_action_stringize (TransportAction * self)
{
  switch (self->type)
    {
    case TRANSPORT_ACTION_BPM_CHANGE:
      return g_strdup (_ ("Change BPM"));
    case TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      return g_strdup (_ ("Beats per bar change"));
    case TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      return g_strdup (_ ("Beat unit change"));
    }

  g_return_val_if_reached (NULL);
}

void
transport_action_free (TransportAction * self)
{
  free (self);
}
