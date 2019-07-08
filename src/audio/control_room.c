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

/**
 * Inits the ControlRoom.
 */
void
control_room_init (
  ControlRoom * self)
{
  /* Init main fader */
  fader_init (&self->vol_fader,
              FADER_TYPE_GENERIC,
              NULL);

  /* init listen vol fader */
  fader_init (&self->listen_vol_fader,
              FADER_TYPE_GENERIC,
              NULL);
  fader_set_amp (&self->listen_vol_fader, 0.1);
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
