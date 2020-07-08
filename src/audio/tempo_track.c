/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/automation_track.h"
#include "audio/port.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * Inits the tempo track.
 */
void
tempo_track_init (
  Track * self)
{
  self->type = TRACK_TYPE_TEMPO;
  self->main_height = TRACK_DEF_HEIGHT / 2;

  gdk_rgba_parse (&self->color, "#2f6c52");

  /* create bpm port */
  self->bpm_port =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, _("BPM"));
  self->bpm_port->minf = 60.f;
  self->bpm_port->maxf = 360.f;
  self->bpm_port->deff = 140.f;
  port_set_control_value (
    self->bpm_port, self->bpm_port->deff, false,
    false);
  port_set_owner_track (self->bpm_port, self);
  self->bpm_port->id.flags |= PORT_FLAG_BPM;
  self->bpm_port->id.flags |=
    PORT_FLAG_AUTOMATABLE;

  /* create time sig port */
  self->time_sig_port =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, _("Time Signature"));
  port_set_control_value (
    self->time_sig_port, 0.f, false, false);
  port_set_owner_track (self->time_sig_port, self);
  self->time_sig_port->id.flags |=
    PORT_FLAG_TIME_SIG;
  self->time_sig_port->id.flags |=
    PORT_FLAG_AUTOMATABLE;
}

/**
 * Creates the default tempo track.
 */
Track *
tempo_track_default (
  int   track_pos)
{
  Track * self =
    track_new (
      TRACK_TYPE_TEMPO, track_pos, _("Tempo"),
      F_WITHOUT_LANE);

  return self;
}

/**
 * Returns the BPM at the given pos.
 */
bpm_t
tempo_track_get_bpm_at_pos (
  Track *    self,
  Position * pos)
{
  AutomationTrack * at =
    automation_track_find_from_port_id (
      &self->bpm_port->id, false);
  return
    automation_track_get_val_at_pos (
      at, pos, false);
}

/**
 * Returns the current BPM.
 */
bpm_t
tempo_track_get_current_bpm (
  Track * self)
{
  return
    port_get_control_value (
      self->bpm_port, false);
}

/**
 * Sets the BPM.
 *
 * @param update_snap_points Whether to update the
 *   snap points.
 * @param stretch_audio_region Whether to stretch
 *   audio regions. This should only be true when
 *   the BPM change is final.
 * @param start_bpm The BPM at the start of the
 *   action, if not temporary.
 */
void
tempo_track_set_bpm (
  Track * self,
  bpm_t   bpm,
  bpm_t   start_bpm,
  bool    temporary,
  bool    fire_events)
{
  g_message (
    "%s: bpm <%f>, temporary <%d>",
    __func__, (double) bpm, temporary);

  if (bpm < TRANSPORT_MIN_BPM)
    {
      bpm = TRANSPORT_MIN_BPM;
    }
  else if (bpm > TRANSPORT_MAX_BPM)
    {
      bpm = TRANSPORT_MAX_BPM;
    }

  port_set_control_value (
    self->bpm_port, bpm, false, false);

  if (!temporary)
    {
      UndoableAction * action =
        transport_action_new_bpm_change (
          start_bpm,
          port_get_control_value (
            self->bpm_port, false),
          false);
      undo_manager_perform (
        UNDO_MANAGER, action);
    }

  if (fire_events)
    {
      EVENTS_PUSH (ET_BPM_CHANGED, NULL);
    }
}

/**
 * Removes all objects from the tempo track.
 *
 * Mainly used in testing.
 */
void
tempo_track_clear (
  Track * self)
{
  /* TODO */
  /*g_warn_if_reached ();*/
}
