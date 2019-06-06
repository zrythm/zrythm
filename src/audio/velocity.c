/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "audio/velocity.h"
#include "gui/widgets/velocity.h"

/**
 * default velocity
 */
#define DEFAULT_VELOCITY 90

Velocity *
velocity_new (int        vel)
{
  Velocity * self = calloc (1, sizeof (Velocity));

  self->vel = vel;

  return self;
}

Velocity *
velocity_clone (Velocity * src)
{
  return velocity_new (src->vel);
}

Velocity *
velocity_default ()
{
  Velocity * self = calloc (1, sizeof (Velocity));

  self->vel = DEFAULT_VELOCITY;

  return self;
}

void
velocity_free (Velocity * self)
{
  if (self->widget)
    gtk_widget_destroy (
      GTK_WIDGET (self->widget));

  free (self);
}
