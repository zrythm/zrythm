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

#include <stdlib.h>

#include "audio/midi_note.h"
#include "audio/velocity.h"
#include "gui/widgets/velocity.h"

#include <gtk/gtk.h>


void
velocity_init_loaded (
  Velocity * self)
{
  ARRANGER_OBJECT_SET_AS_MAIN (
    VELOCITY, Velocity, velocity);

  velocity_set_midi_note (self, self->midi_note);
}

/**
 * Creates a new Velocity with the given value.
 */
Velocity *
velocity_new (
  MidiNote * midi_note,
  int        vel,
  int        is_main)
{
  Velocity * self = calloc (1, sizeof (Velocity));

  self->vel = vel;
  self->midi_note = midi_note;

  if (is_main)
    {
      ARRANGER_OBJECT_SET_AS_MAIN (
        VELOCITY, Velocity, velocity);
    }

  return self;
}

/**
 * Sets the MidiNote the Velocity belongs to.
 */
void
velocity_set_midi_note (
  Velocity * velocity,
  MidiNote * midi_note)
{
  Velocity * vel;
  for (int i = 0; i < 2; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        vel =
          velocity_get_main_velocity (velocity);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        vel =
          velocity_get_main_trans_velocity (
            velocity);

      vel->midi_note = midi_note;
    }
}

/**
 * Finds the actual Velocity in the project from the
 * given clone.
 */
Velocity *
velocity_find (
  Velocity * clone)
{
  MidiNote * mn =
    midi_note_find (clone->midi_note);
  g_return_val_if_fail (mn && mn->vel, NULL);

  return mn->vel;
}

/**
 * Clones the Velocity.
 */
Velocity *
velocity_clone (
  Velocity * src,
  VelocityCloneFlag flag)
{
  int is_main = 0;
  if (flag == VELOCITY_CLONE_COPY_MAIN)
    is_main = 1;

  Velocity * vel =
    velocity_new (
      src->midi_note, src->vel, is_main);

  return vel;
}

/**
 * Returns 1 if the Velocity's match, 0 if not.
 */
int
velocity_is_equal (
  Velocity * src,
  Velocity * dest)
{
  return
    src->vel == dest->vel &&
    midi_note_is_equal (
      src->midi_note, dest->midi_note);
}

/**
 * Sets the cached value for use in live actions.
 */
void
velocity_set_cache_vel (
  Velocity * velocity,
  const int  vel)
{
  /* see ARRANGER_OBJ_SET_POS */
  velocity_get_main_velocity (velocity)->
    cache_vel = vel;
  velocity_get_main_trans_velocity (velocity)->
    cache_vel = vel;
}

/**
 * Sets the velocity to the given value.
 */
void
velocity_set_val (
  Velocity * self,
  int        val)
{
  /* re-set the midi note value to set a note off
   * event */
  self->vel = CLAMP (val, 0, 127);
  midi_note_set_val (self->midi_note,
                     self->midi_note->val);
}

ARRANGER_OBJ_DEFINE_GEN_WIDGET_LANELESS (
  Velocity, velocity);

/**
 * Changes the Velocity by the given amount of
 * values (delta).
 */
void
velocity_shift (
  Velocity * self,
  int        delta)
{
  self->vel += delta;
}

ARRANGER_OBJ_DEFINE_FREE_ALL_LANELESS (
  Velocity, velocity);

void
velocity_free (Velocity * self)
{
  if (self->widget)
    gtk_widget_destroy (
      GTK_WIDGET (self->widget));

  free (self);
}
