/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * FIXME make a general port change action.
 */
UndoableAction *
transport_action_new_bpm_change (
  bpm_t           bpm_before,
  bpm_t           bpm_after,
  bool            already_done)
{
  TransportAction * self =
    calloc (1, sizeof (TransportAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_TRANSPORT;

  self->bpm_before = bpm_before;
  self->bpm_after = bpm_after;
  self->already_done = already_done;
  self->musical_mode =
    g_settings_get_boolean (S_UI, "musical-mode");

  return ua;
}

static int
do_or_undo (
  TransportAction * self,
  bool              _do)
{
  port_set_control_value (
    P_TEMPO_TRACK->bpm_port,
    _do ? self->bpm_after : self->bpm_before,
    false, false);
  g_message ("set BPM to %f",
    (double)
    tempo_track_get_current_bpm (P_TEMPO_TRACK));
  engine_update_frames_per_tick (
    AUDIO_ENGINE, TRANSPORT->beats_per_bar,
    tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate);

  snap_grid_update_snap_points (SNAP_GRID_TIMELINE);
  snap_grid_update_snap_points (SNAP_GRID_MIDI);

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

  if (self->musical_mode)
    {
      transport_stretch_audio_regions (
        TRANSPORT, NULL, true, time_ratio);
    }

  return 0;
}

int
transport_action_do (
  TransportAction * self)
{
  if (self->already_done)
    {
      self->already_done = false;
    }
  else
    {
      do_or_undo (self, true);
    }

  EVENTS_PUSH (ET_BPM_CHANGED, NULL);

  return 0;
}

int
transport_action_undo (
  TransportAction * self)
{
  do_or_undo (self, false);

  EVENTS_PUSH (ET_BPM_CHANGED, NULL);

  return 0;
}

char *
transport_action_stringize (
  TransportAction * self)
{
  return g_strdup (_("Change BPM"));
}

void
transport_action_free (
  TransportAction * self)
{
  free (self);
}
