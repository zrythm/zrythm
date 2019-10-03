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

#include "audio/automation_curve.h"
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
  int ret =
    position_compare (
      &a->pos, &b->pos);
  if (ret == 0 &&
      a->index <
      b->index)
    {
      return -1;
    }

  return ret;
}

static int
cmpfunc_curve (const void * _a, const void * _b)
{
  AutomationCurve * a =
    *(AutomationCurve * const *)_a;
  AutomationCurve * b =
    *(AutomationCurve * const *)_b;
  int ret = position_compare (
    &a->pos, &b->pos);
  if (ret == 0 &&
      a->index <
      b->index)
    {
      return -1;
    }

  return ret;
}

Region *
automation_region_new (
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main)
{
  Region * self =
    calloc (1, sizeof (Region));

  self->type = REGION_TYPE_AUTOMATION;

  self->aps_size = 2;
  self->aps =
    malloc (self->aps_size *
            sizeof (AutomationPoint *));
  self->aps =
    malloc ((self->aps_size - 1) *
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
  Region * self)
{
  AutomationPoint * ap;
  for (int i = 0; i < self->num_aps; i++)
    {
      ap = self->aps[i];
      g_message ("%d", i);
      position_print_yaml (&ap->pos);
    }
}

/**
 * Forces sort of the automation points.
 */
void
automation_region_force_sort (
  Region * self)
{
  /* sort by position */
  qsort (self->aps,
         (size_t) self->num_aps,
         sizeof (AutomationPoint *),
         cmpfunc);
  qsort (self->acs,
         (size_t) self->num_acs,
         sizeof (AutomationCurve *),
         cmpfunc_curve);

  /* refresh indices */
  for (int i = 0;
       i < self->num_acs;
       i++)
    self->acs[i]->index = i;

  /* refresh indices */
  for (int i = 0; i < self->num_aps; i++)
    automation_point_set_region_and_index (
      self->aps[i], self, i);
}

/**
 * Adds the given AutomationCurve.
 */
void
automation_region_add_ac (
  Region *          self,
  AutomationCurve * ac)
{
  /** double array size if full */
  if ((size_t) self->num_acs == self->aps_size - 1)
    {
      self->aps_size =
        self->aps_size == 0 ? 1 :
        self->aps_size * 2;
      self->acs =
        realloc (
          self->acs,
          sizeof (AutomationCurve *) *
          (self->aps_size - 1));
    }

  /* add point */
  array_append (
    self->acs, self->num_acs, ac);
  ac->region = self;

  /* sort by position */
  qsort (self->acs,
         (size_t) self->num_acs,
         sizeof (AutomationCurve *),
         cmpfunc_curve);
  g_warn_if_fail (
    automation_region_get_ap_before_curve (self, ac)->pos.total_ticks <
    automation_region_get_ap_after_curve (self, ac)->pos.total_ticks);

  /* refresh indices */
  for (int i = 0;
       i < self->num_acs;
       i++)
    self->acs[i]->index = i;

  EVENTS_PUSH (ET_AUTOMATION_CURVE_CREATED, ac);
}

static void
create_curve_point_before (
  Region * self,
  AutomationPoint * prev_ap,
  AutomationPoint * ap)
{
  Position mid_pos;

  /* get midpoint between prev ap and current */
  position_get_midway_pos (&prev_ap->pos,
                           &ap->pos,
                           &mid_pos);
  g_warn_if_fail (
    position_is_before_or_equal (
      &mid_pos, &ap->pos));
  g_warn_if_fail (
    position_is_after_or_equal (
      &mid_pos, &prev_ap->pos));

  /* create curve point at mid pos */
  AutomationCurve * curve =
    automation_curve_new (
      self->at->automatable->type, &mid_pos);
  automation_region_add_ac (self, curve);
}

static void
create_curve_point_after (
  Region * self,
  AutomationPoint * next_ap,
  AutomationPoint * ap)
{
  Position mid_pos;

  /* get midpoint between prev ap and current */
  position_get_midway_pos (&ap->pos,
                           &next_ap->pos,
                           &mid_pos);

  /* create curve point at mid pos */
  AutomationCurve * curve =
    automation_curve_new (
      self->at->automatable->type, &mid_pos);
  automation_region_add_ac (self, curve);
}

/**
 * Adds automation point and optionally generates
 * curve points accordingly.
 *
 * @param gen_curve_points Generate AutomationCurve's.
 */
void
automation_region_add_ap (
  Region *          self,
  AutomationPoint * ap,
  int               gen_curves)
{
  g_return_if_fail (
    ap ==
    automation_point_get_main_automation_point (ap));

  /* add point */
  array_double_size_if_full (
    self->aps, self->num_aps, self->aps_size,
    AutomationPoint *);
  self->acs =
    realloc (
      self->acs,
      sizeof (AutomationCurve *) *
      (self->aps_size - 1));
  array_append (
    self->aps, self->num_aps, ap);

  /* sort by position */
  qsort (self->aps,
         (size_t) self->num_aps,
         sizeof (AutomationPoint *),
         cmpfunc);
  if (self->num_aps > 1)
    {
      g_warn_if_fail (
        self->aps[self->num_aps - 1]->pos.total_ticks >
        self->aps[self->num_aps - 2]->pos.total_ticks);
    }

  /* refresh indices */
  for (int i = 0; i < self->num_aps; i++)
    automation_point_set_region_and_index (
      self->aps[i], self, i);

  if (gen_curves)
    {
      /* add midway automation curve to control
       * curviness */
      AutomationPoint * prev_ap =
        automation_region_get_prev_ap (self, ap);
      AutomationPoint * next_ap =
        automation_region_get_next_ap (self, ap);
      /* 4 cases */
      if (!prev_ap && !next_ap) /* only ap */
        {
          /* do nothing */
        }
      else if (prev_ap && !next_ap) /* last ap */
        {
          create_curve_point_before (
            self, prev_ap, ap);
        }
      else if (!prev_ap && next_ap) /* first ap */
        {
          create_curve_point_after (
            self, next_ap, ap);
        }
      else  /* has next & prev */
        {
          /* remove existing curve */
          AutomationCurve * prev_curve =
            automation_region_get_next_curve_ac (
              self, prev_ap);
          automation_region_remove_ac (
            self, prev_curve, F_FREE);

          create_curve_point_before (
            self, prev_ap, ap);
          create_curve_point_after (
            self, next_ap, ap);
        }
    }

  EVENTS_PUSH (ET_AUTOMATION_POINT_CREATED,
               ap);
}

AutomationPoint *
automation_region_get_ap_before_curve (
  Region *          self,
  AutomationCurve * ac)
{
  if (ac && ac->index < self->num_aps)
    return self->aps[ac->index];

  return NULL;
}

/**
 * Returns the ap after the curve point.
 */
AutomationPoint *
automation_region_get_ap_after_curve (
  Region *          self,
  AutomationCurve * ac)
{
  if (ac &&
      ac->index < self->num_aps - 1)
    return self->aps[ac->index + 1];

  return NULL;
}

/**
 * Returns the automation point before the position.
 */
AutomationPoint *
automation_region_get_prev_ap (
  Region *          self,
  AutomationPoint * ap)
{
  if (ap->index > 0)
    return self->aps[ap->index - 1];

  return NULL;
}

/**
 * Returns the automation point after the position.
 */
AutomationPoint *
automation_region_get_next_ap (
  Region *          self,
  AutomationPoint * ap)
{
  if (ap && ap->index < self->num_aps - 1)
    return self->aps[ap->index + 1];

  return NULL;
}

/**
 * Returns the curve point right after the given ap
 */
AutomationCurve *
automation_region_get_next_curve_ac (
  Region *          self,
  AutomationPoint * ap)
{
  /* if not last or only AP */
  if (ap->index != self->num_aps - 1)
    return self->acs[ap->index];

  return NULL;
}

/**
 * Removes the AutomationPoint from the Region,
 * optionally freeing it.
 */
void
automation_region_remove_ap (
  Region *          self,
  AutomationPoint * ap,
  int               free)
{
  /* deselect */
  automation_selections_remove_automation_point (
    AUTOMATION_SELECTIONS, ap);

  /* remove the curves first */
  AutomationPoint * prev_ap =
    automation_region_get_prev_ap (self, ap);
  AutomationPoint * next_ap =
    automation_region_get_next_ap (self, ap);

  AutomationCurve * ac;
  /* 4 cases */
  if (!prev_ap && !next_ap) /* only ap */
    {
      /* do nothing */
    }
  else if (prev_ap && !next_ap) /* last ap */
    {
      /* remove curve before */
      ac =
        automation_region_get_next_curve_ac (
          self, prev_ap);
      automation_region_remove_ac (self, ac, F_FREE);
    }
  else if (!prev_ap && next_ap) /* first ap */
    {
      /* remove curve after */
      ac =
        automation_region_get_next_curve_ac (
          self, ap);
      automation_region_remove_ac (self, ac, F_FREE);
    }
  else  /* has next & prev */
    {
      /* remove curve before and after, and add
       * a new curve at this ap's pos */
      ac =
        automation_region_get_next_curve_ac (
          self, prev_ap);
      automation_region_remove_ac (self, ac, F_FREE);
      ac =
        automation_region_get_next_curve_ac (
          self, ap);
      automation_region_remove_ac (self, ac, F_FREE);

      create_curve_point_before (
        self, prev_ap, next_ap);
    }

  array_delete (self->aps,
                self->num_aps,
                ap);


  if (free)
    free_later (ap, automation_point_free);

  EVENTS_PUSH (ET_AUTOMATION_POINT_REMOVED, NULL);
}

/**
 * Removes the AutomationCurve from the Region,
 * optionally freeing it.
 */
void
automation_region_remove_ac (
  Region *          self,
  AutomationCurve * ac,
  int               free)
{
  array_delete (self->acs,
                self->num_acs,
                ac);
  g_warn_if_fail (ac);
  if (free)
    free_later (
      ac, automation_curve_free);

  EVENTS_PUSH (ET_AUTOMATION_CURVE_REMOVED, NULL);
}

/**
 * Frees members only but not the Region itself.
 *
 * Regions should be free'd using region_free.
 */
void
automation_region_free_members (
  Region * self)
{
  int i;
  for (i = 0; i < self->num_aps; i++)
    automation_region_remove_ap (
      self, self->aps[i], F_FREE);

  for (i = 0; i < self->num_acs; i++)
    automation_region_remove_ac (
      self, self->acs[i], F_FREE);

  free (self->aps);
  free (self->acs);
}
