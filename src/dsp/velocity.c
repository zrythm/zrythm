/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "dsp/midi_note.h"
#include "dsp/region.h"
#include "dsp/velocity.h"
#include "gui/widgets/velocity.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/types.h"

#include <gtk/gtk.h>

/**
 * Creates a new Velocity with the given value.
 */
Velocity *
velocity_new (MidiNote * midi_note, const uint8_t vel)
{
  Velocity * self = object_new (Velocity);

  self->schema_version = VELOCITY_SCHEMA_VERSION;
  ArrangerObject * obj = (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_VELOCITY;

  self->vel = vel;
  self->midi_note = midi_note;

  arranger_object_init (obj);

  return self;
}

/**
 * Sets the MidiNote the Velocity belongs to.
 */
void
velocity_set_midi_note (Velocity * self, MidiNote * midi_note)
{
  self->midi_note = midi_note;
}

/**
 * Returns 1 if the Velocity's match, 0 if not.
 */
int
velocity_is_equal (Velocity * src, Velocity * dest)
{
  ArrangerObject * src_mn_obj =
    (ArrangerObject *) src->midi_note;
  ArrangerObject * dest_mn_obj =
    (ArrangerObject *) dest->midi_note;
  return src->vel == dest->vel
         && src->midi_note->pos == dest->midi_note->pos
         && region_identifier_is_equal (
           &src_mn_obj->region_id, &dest_mn_obj->region_id);
}

/**
 * Sets the velocity to the given value.
 *
 * The given value may exceed the bounds 0-127,
 * and will be clamped.
 */
void
velocity_set_val (Velocity * self, const int val)
{
  g_return_if_fail (IS_ARRANGER_OBJECT (self));

  self->vel = (uint8_t) CLAMP (val, 0, 127);

  /* re-set the midi note value to set a note off
   * event */
  MidiNote * note = velocity_get_midi_note (self);
  g_return_if_fail (IS_MIDI_NOTE (note));
  midi_note_set_val (note, note->val);
}

/**
 * Changes the Velocity by the given amount of
 * values (delta).
 */
void
velocity_shift (Velocity * self, const int delta)
{
  self->vel = (midi_byte_t) ((int) self->vel + delta);
}

/**
 * Returns the owner MidiNote.
 */
MidiNote *
velocity_get_midi_note (const Velocity * const self)
{
  g_return_val_if_fail (IS_MIDI_NOTE (self->midi_note), NULL);

  return self->midi_note;

#if 0
  g_return_val_if_fail (self, NULL);
  ZRegion * region =
    region_find (&self->region_id);
  g_return_val_if_fail (region, NULL);
  return region->midi_notes[self->note_pos];
#endif
}

const char *
velocity_setting_enum_to_str (guint index)
{
  switch (index)
    {
    case 0:
      return "last-note";
    case 1:
      return "40";
    case 2:
      return "90";
    case 3:
      return "120";
    default:
      break;
    }

  g_return_val_if_reached (NULL);
}

guint
velocity_setting_str_to_enum (const char * str)
{
  guint val;
  if (string_is_equal (str, "40"))
    {
      val = 1;
    }
  else if (string_is_equal (str, "90"))
    {
      val = 2;
    }
  else if (string_is_equal (str, "120"))
    {
      val = 3;
    }
  else
    {
      val = 0;
    }

  return val;
}
