// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>
#include <stdlib.h>

#include "dsp/chord_object.h"
#include "dsp/chord_track.h"
#include "dsp/position.h"
#include "gui/widgets/chord_object.h"
#include "project.h"
#include "utils/debug.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

/**
 * Creates a ChordObject.
 */
ChordObject *
chord_object_new (RegionIdentifier * region_id, int chord_index, int index)
{
  ChordObject * self = object_new (ChordObject);

  ArrangerObject * obj = (ArrangerObject *) self;
  obj->type = ArrangerObjectType::ARRANGER_OBJECT_TYPE_CHORD_OBJECT;

  self->chord_index = chord_index;
  self->index = index;
  region_identifier_copy (&obj->region_id, region_id);
  self->magic = CHORD_OBJECT_MAGIC;

  arranger_object_init (obj);

  return self;
}

/**
 * Returns the ChordDescriptor associated with this
 * ChordObject.
 */
ChordDescriptor *
chord_object_get_chord_descriptor (const ChordObject * self)
{
  g_return_val_if_fail (CLIP_EDITOR, NULL);
  z_return_val_if_fail_cmp (self->chord_index, >=, 0, NULL);
  return CHORD_EDITOR->chords[self->chord_index];
}

int
chord_object_is_equal (ChordObject * a, ChordObject * b)
{
  ArrangerObject * obj_a = (ArrangerObject *) a;
  ArrangerObject * obj_b = (ArrangerObject *) b;
  return position_is_equal_ticks (&obj_a->pos, &obj_b->pos)
         && a->chord_index == b->chord_index && a->index == b->index;
}

/**
 * Finds the ChordObject in the project
 * corresponding to the given one's position.
 *
 * Used in actions.
 */
ChordObject *
chord_object_find_by_pos (ChordObject * clone)
{
  ArrangerObject * clone_obj = (ArrangerObject *) clone;

  /* get actual region - clone's region might be
   * an unused clone */
  ZRegion * r = region_find (&clone_obj->region_id);
  g_return_val_if_fail (r, NULL);

  ChordObject *    chord;
  ArrangerObject * c_obj;
  for (int i = 0; i < r->num_chord_objects; i++)
    {
      chord = r->chord_objects[i];
      c_obj = (ArrangerObject *) chord;
      if (position_is_equal (&c_obj->pos, &clone_obj->pos))
        return chord;
    }
  return NULL;
}

/**
 * Sets the region and index of the chord.
 */
void
chord_object_set_region_and_index (ChordObject * self, ZRegion * region, int idx)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  region_identifier_copy (&obj->region_id, &region->id);
  self->index = idx;
}

ZRegion *
chord_object_get_region (ChordObject * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  return region_find (&obj->region_id);
}
