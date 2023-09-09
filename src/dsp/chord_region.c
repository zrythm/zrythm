// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_object.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Creates a new ZRegion for chords.
 *
 * @param idx Index inside chord track.
 */
ZRegion *
chord_region_new (const Position * start_pos, const Position * end_pos, int idx)
{
  ZRegion * self = object_new (ZRegion);

  self->chord_objects_size = 1;
  self->chord_objects = object_new_n (self->chord_objects_size, ChordObject *);

  self->id.type = REGION_TYPE_CHORD;

  region_init (
    self, start_pos, end_pos, track_get_name_hash (P_CHORD_TRACK), 0, idx);

  g_warn_if_fail (IS_REGION (self));

  return self;
}

/**
 * Inserts a ChordObject to the Region.
 */
void
chord_region_insert_chord_object (
  ZRegion *     self,
  ChordObject * chord,
  int           pos,
  bool          fire_events)
{
  g_return_if_fail (IS_REGION (self));

  char              str[500];
  ChordDescriptor * cd = chord_object_get_chord_descriptor (chord);
  chord_descriptor_to_string (cd, str);
  g_message (
    "inserting chord '%s' (index %d) to "
    "region '%s' at pos %d",
    str, chord->index, self->name, pos);

  array_double_size_if_full (
    self->chord_objects, self->num_chord_objects, self->chord_objects_size,
    ChordObject *);
  array_insert (self->chord_objects, self->num_chord_objects, pos, chord);

  for (int i = pos; i < self->num_chord_objects; i++)
    {
      ChordObject * co = self->chord_objects[i];
      chord_object_set_region_and_index (co, self, i);
    }

  if (fire_events)
    {
      EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, chord);
    }
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
  chord_region_insert_chord_object (
    self, chord, self->num_chord_objects, fire_events);
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
  g_return_if_fail (IS_REGION (self) && IS_CHORD_OBJECT (chord));

  char              str[500];
  ChordDescriptor * cd = chord_object_get_chord_descriptor (chord);
  chord_descriptor_to_string (cd, str);
  g_message (
    "removing chord '%s' (index %d) from "
    "region '%s'",
    str, chord->index, self->name);

  /* deselect */
  if (CHORD_SELECTIONS)
    {
      arranger_object_select (
        (ArrangerObject *) chord, F_NO_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
    }

  int pos = -1;
  array_delete_return_pos (
    self->chord_objects, self->num_chord_objects, chord, pos);
  g_return_if_fail (pos >= 0);

  for (int i = pos; i < self->num_chord_objects; i++)
    {
      chord_object_set_region_and_index (self->chord_objects[i], self, i);
    }

  if (free)
    {
      arranger_object_free ((ArrangerObject *) chord);
    }

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_REMOVED, ARRANGER_OBJECT_TYPE_CHORD_OBJECT);
    }
}

bool
chord_region_validate (ZRegion * self)
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
chord_region_free_members (ZRegion * self)
{
  g_return_if_fail (IS_REGION (self));

  for (int i = 0; i < self->num_chord_objects; i++)
    {
      chord_region_remove_chord_object (
        self, self->chord_objects[i], F_FREE, F_NO_PUBLISH_EVENTS);
    }

  object_zero_and_free (self->chord_objects);
}
