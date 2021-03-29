/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/chord_region.h"
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Creates a new ZRegion for chords.
 *
 * @param idx Index inside chord track.
 */
ZRegion *
chord_region_new (
  const Position * start_pos,
  const Position * end_pos,
  int              idx)
{
  ZRegion * self = object_new (ZRegion);

  self->chord_objects_size = 1;
  self->chord_objects =
    object_new_n (
      self->chord_objects_size, ChordObject *);

  self->id.type = REGION_TYPE_CHORD;

  region_init (
    self, start_pos, end_pos, P_CHORD_TRACK->pos,
    0, idx);

  g_warn_if_fail (IS_REGION (self));

  return self;
}

/**
 * Adds a ChordObject to the Region.
 */
void
chord_region_add_chord_object (
  ZRegion *     self,
  ChordObject * chord,
  bool          fire_events)
{
  g_return_if_fail (IS_REGION (self));

  array_double_size_if_full (
    self->chord_objects, self->num_chord_objects,
    self->chord_objects_size, ChordObject *);
  array_append (
    self->chord_objects, self->num_chord_objects,
    chord);

  chord_object_set_region_and_index (
    chord, self, self->num_chord_objects - 1);

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_CREATED, chord);
    }
}

/**
 * Removes a ChordObject from the Region.
 *
 * @param free Optionally free the ChordObject.
 */
void
chord_region_remove_chord_object (
  ZRegion *     self,
  ChordObject * chord,
  int           free,
  bool          fire_events)
{
  g_return_if_fail (
    IS_REGION (self) && IS_CHORD_OBJECT (chord));

  /* deselect */
  if (CHORD_SELECTIONS)
    {
      arranger_object_select (
        (ArrangerObject *) chord, F_NO_SELECT,
        F_APPEND,
        F_NO_PUBLISH_EVENTS);
    }

  array_delete (
    self->chord_objects, self->num_chord_objects,
    chord);

  for (int i = 0; i < self->num_chord_objects; i++)
    {
      chord_object_set_region_and_index (
        self->chord_objects[i], self, i);
    }

  if (free)
    {
      free_later (chord, arranger_object_free);
    }

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_REMOVED,
        ARRANGER_OBJECT_TYPE_CHORD_OBJECT);
    }
}

bool
chord_region_validate (
  ZRegion * self)
{
  for (int i = 0; i < self->num_chord_objects; i++)
    {
      ChordObject * c = self->chord_objects[i];

      g_return_val_if_fail (c->index == i, false);
    }

  return true;
}

/**
 * Frees members only but not the ZRegion itself.
 *
 * Regions should be free'd using region_free.
 */
void
chord_region_free_members (
  ZRegion * self)
{
  g_return_if_fail (IS_REGION (self));

  for (int i = 0; i < self->num_chord_objects; i++)
    {
      chord_region_remove_chord_object (
        self, self->chord_objects[i], F_FREE,
        F_NO_PUBLISH_EVENTS);
    }

  object_zero_and_free (self->chord_objects);
}
