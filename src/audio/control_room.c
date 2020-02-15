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

#include "audio/control_room.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/control_room.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "settings/settings.h"
#include "zrythm.h"

/**
 * Inits the ControlRoom.
 *
 * @param loading 1 if loading.
 */
void
control_room_init (
  ControlRoom * self,
  int           loading)
{
  /* Init main fader */
  if (loading)
    {
      self->monitor_fader.stereo_in->
        l->id.owner_type =
        PORT_OWNER_TYPE_MONITOR_FADER;
      self->monitor_fader.stereo_in->
        r->id.owner_type =
        PORT_OWNER_TYPE_MONITOR_FADER;
      self->monitor_fader.stereo_out->
        l->id.owner_type =
        PORT_OWNER_TYPE_MONITOR_FADER;
      self->monitor_fader.stereo_out->
        r->id.owner_type =
        PORT_OWNER_TYPE_MONITOR_FADER;
    }
  else
    {
      fader_init (
        &self->monitor_fader,
        FADER_TYPE_MONITOR,
        NULL);
    }

  /* set the monitor volume */
  float amp =
    ZRYTHM_TESTING ?
      1.f :
      (float)
      g_settings_get_double (
        S_UI, "monitor-out-vol");
  fader_set_amp (
    &self->monitor_fader, amp);

  /* init listen vol fader */
  fader_init (&self->listen_vol_fader,
              FADER_TYPE_GENERIC,
              NULL);
  fader_set_amp (&self->listen_vol_fader, 0.1f);
}

/**
 * Sets dim_output to on/off and notifies interested
 * parties.
 */
void
control_room_set_dim_output (
  ControlRoom * self,
  int           dim_output)
{
  self->dim_output = dim_output;
}
