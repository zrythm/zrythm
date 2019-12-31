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

#include "audio/automation_point.h"
#include "audio/automation_region.h"
#include "audio/position.h"
#include "audio/region.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/events.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

static int
cmpfunc (const void * _a, const void * _b)
{
  AutomationPoint * a =
    *(AutomationPoint * const *) _a;
  AutomationPoint * b =
    *(AutomationPoint * const *)_b;
  ArrangerObject * a_obj =
    (ArrangerObject *) a;
  ArrangerObject * b_obj =
    (ArrangerObject *) b;
  long ret =
    position_compare (
      &a_obj->pos, &b_obj->pos);
  if (ret == 0 &&
      a->index <
      b->index)
    {
      return -1;
    }

  return (int) CLAMP (ret, -1, 1);
}

ZRegion *
automation_region_new (
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main)
{
  ZRegion * self =
    calloc (1, sizeof (ZRegion));

  self->type = REGION_TYPE_AUTOMATION;

  self->aps_size = 2;
  self->aps =
    malloc (self->aps_size *
            sizeof (AutomationPoint *));

  region_init (
    self, start_pos, end_pos, is_main);

  return self;
}

/**
 * Prints the automation in this Region.
 */
void
automation_region_print_automation (
  ZRegion * self)
{
  AutomationPoint * ap;
  ArrangerObject * ap_obj;
  for (int i = 0; i < self->num_aps; i++)
    {
      ap = self->aps[i];
      ap_obj = (ArrangerObject *) ap;
      g_message ("%d", i);
      position_print_yaml (&ap_obj->pos);
    }
}

/**
 * Forces sort of the automation points.
 */
void
automation_region_force_sort (
  ZRegion * self)
{
  /* sort by position */
  qsort (self->aps,
         (size_t) self->num_aps,
         sizeof (AutomationPoint *),
         cmpfunc);

  /* refresh indices */
  for (int i = 0; i < self->num_aps; i++)
    {
      automation_point_set_region_and_index (
        self->aps[i], self, i);
    }
}

/**
 * Adds an AutomationPoint to the Region.
 */
void
automation_region_add_ap (
  ZRegion *          self,
  AutomationPoint * ap)
{
  /* add point */
  array_double_size_if_full (
    self->aps, self->num_aps, self->aps_size,
    AutomationPoint *);
  array_append (
    self->aps, self->num_aps, ap);

  /* sort by position */
  qsort (
    self->aps, (size_t) self->num_aps,
    sizeof (AutomationPoint *), cmpfunc);

  /* refresh indices */
  for (int i = 0; i < self->num_aps; i++)
    {
      automation_point_set_region_and_index (
        self->aps[i], self, i);
    }

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CREATED, ap);
}


/**
 * Returns the AutomationPoint before the given
 * one.
 */
AutomationPoint *
automation_region_get_prev_ap (
  ZRegion *          self,
  AutomationPoint * ap)
{
  if (ap->index > 0)
    return self->aps[ap->index - 1];

  return NULL;
}

/**
 * Returns the AutomationPoint after the given
 * one.
 */
AutomationPoint *
automation_region_get_next_ap (
  ZRegion *          self,
  AutomationPoint * ap)
{
  g_return_val_if_fail (
    self && ap, NULL);

  if (ap->index < self->num_aps - 1)
    return self->aps[ap->index + 1];

  return NULL;
}

/**
 * Removes the AutomationPoint from the ZRegion,
 * optionally freeing it.
 *
 * @param free Free the AutomationPoint after
 *   removing it.
 */
void
automation_region_remove_ap (
  ZRegion *          self,
  AutomationPoint * ap,
  int               free)
{
  /* deselect */
  arranger_object_select (
    (ArrangerObject *) ap, F_NO_SELECT,
    F_APPEND);

  array_delete (
    self->aps, self->num_aps, ap);

  if (free)
    free_later (ap, arranger_object_free);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_REMOVED,
    ARRANGER_OBJECT_TYPE_AUTOMATION_POINT);
}

/**
 * Frees members only but not the ZRegion itself.
 *
 * Regions should be free'd using region_free.
 */
void
automation_region_free_members (
  ZRegion * self)
{
  int i;
  for (i = 0; i < self->num_aps; i++)
    automation_region_remove_ap (
      self, self->aps[i], F_FREE);

  free (self->aps);
}
