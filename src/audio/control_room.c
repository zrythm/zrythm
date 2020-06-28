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
#include "audio/fader.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/control_room.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "settings/settings.h"
#include "utils/objects.h"
#include "zrythm.h"

static void
init_common (
  ControlRoom * self)
{
  /* set the monitor volume */
  float amp =
    ZRYTHM_TESTING ?
      1.f :
      (float)
      g_settings_get_double (
        S_UI, "monitor-out-vol");
  fader_set_amp (self->monitor_fader, amp);

  /* init listen vol fader */
  self->listen_vol_fader =
    fader_new (
      FADER_TYPE_GENERIC, NULL, false);
  fader_set_amp (self->listen_vol_fader, 0.1f);
  fader_set_is_project (
    self->listen_vol_fader, true);
}

/**
 * Inits the control room from a project.
 */
void
control_room_init_loaded (
  ControlRoom * self)
{
  fader_init_loaded (self->monitor_fader, true);

  init_common (self);
}

/**
 * Creates a new control room.
 */
ControlRoom *
control_room_new (void)
{
  ControlRoom * self = object_new (ControlRoom);

  self->monitor_fader =
    fader_new (
      FADER_TYPE_MONITOR, NULL, false);
  fader_set_is_project (self->monitor_fader, true);

  init_common (self);

  return self;
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

void
control_room_free (
  ControlRoom * self)
{
  object_free_w_func_and_null (
    fader_free, self->monitor_fader);
  object_free_w_func_and_null (
    fader_free, self->listen_vol_fader);

  object_zero_and_free (self);
}
