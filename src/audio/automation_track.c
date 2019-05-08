/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_curve.h"
#include "audio/automation_track.h"
#include "audio/automation_point.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/objects.h"

void
automation_track_init_loaded (
  AutomationTrack * self)
{
  self->automatable =
    automatable_find (self->automatable);

  /* TODO track */
  /*self->track =*/
    /*project_get_track (*/
      /*self->track_id);*/

  /* TODO al */
  /*self->al =*/
    /*project_get_automation_lane (self->al_id);*/
}

AutomationTrack *
automation_track_new (Automatable *   a)
{
  AutomationTrack * at =
    calloc (1, sizeof (AutomationTrack));

  at->track = a->track;
  at->automatable = a;

  return at;
}

static int
cmpfunc (const void * _a, const void * _b)
{
  AutomationPoint * a = *(AutomationPoint **)_a;
  AutomationPoint * b = *(AutomationPoint **)_b;
  int ret = position_compare (&a->pos,
                              &b->pos);
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
  AutomationCurve * a = *(AutomationCurve **)_a;
  AutomationCurve * b = *(AutomationCurve **)_b;
  int ret = position_compare (&a->pos,
                              &b->pos);
  if (ret == 0 &&
      a->index <
      b->index)
    {
      return -1;
    }

  return ret;
}

/**
 * Adds and shows curve point at pos.
 */
static void
add_and_show_curve_point (AutomationTrack * at,
                          Position        * pos)
{
  /* create curve point at mid pos */
  AutomationCurve * curve =
    automation_curve_new (at, pos);
  automation_track_add_ac (at, curve);

  /* FIXME these should be in gui code */
  gtk_overlay_add_overlay (GTK_OVERLAY (MW_TIMELINE),
                           GTK_WIDGET (curve->widget));
  gtk_widget_show (GTK_WIDGET (curve->widget));
}

static void
create_curve_point_before (AutomationTrack * at,
                           AutomationPoint * prev_ap,
                           AutomationPoint * ap)
{
  Position mid_pos;

  /* get midpoint between prev ap and current */
  position_get_midway_pos (&prev_ap->pos,
                           &ap->pos,
                           &mid_pos);
  g_warn_if_fail (position_compare (&mid_pos,
                              &ap->pos) <= 0);
  g_warn_if_fail (position_compare (&mid_pos,
                              &prev_ap->pos) >= 0);

  add_and_show_curve_point (at, &mid_pos);
}

static void
create_curve_point_after (AutomationTrack * at,
                           AutomationPoint * next_ap,
                           AutomationPoint * ap)
{
  Position mid_pos;

  /* get midpoint between current ap and next */
  position_get_midway_pos (&ap->pos,
                           &next_ap->pos,
                           &mid_pos);

  add_and_show_curve_point (at, &mid_pos);
}

void
automation_track_print_automation_points (AutomationTrack * at)
{
  for (int i = 0; i < at->num_aps; i++)
    {
      AutomationPoint * ap = at->aps[i];
      g_message ("%d", i);
      position_print (&ap->pos);
    }
}

void
automation_track_force_sort (AutomationTrack * at)
{
  /* sort by position */
  qsort (at->aps,
         at->num_aps,
         sizeof (AutomationPoint *),
         cmpfunc);
  qsort (at->acs,
         at->num_acs,
         sizeof (AutomationCurve *),
         cmpfunc_curve);

  /* refresh indices */
  for (int i = 0;
       i < at->num_acs;
       i++)
    at->acs[i]->index = i;

  /* refresh indices */
  for (int i = 0;
       i < at->num_aps;
       i++)
    at->aps[i]->index = i;
}

/**
 * Adds automation curve.
 */
void
automation_track_add_ac (
  AutomationTrack * at,
  AutomationCurve * ac)
{
  /* add point */
  array_append (at->acs,
                at->num_acs,
                ac);

  /* sort by position */
  qsort (at->acs,
         at->num_acs,
         sizeof (AutomationCurve *),
         cmpfunc_curve);

  /* refresh indices */
  for (int i = 0;
       i < at->num_acs;
       i++)
    at->acs[i]->index = i;
}

/**
 * Adds automation point and optionally generates curve points accordingly.
 */
void
automation_track_add_ap (
  AutomationTrack * at,
  AutomationPoint * ap,
  int               generate_curve_points)
{
  /* add point */
  array_append (at->aps,
                at->num_aps,
                ap);

  /* sort by position */
  qsort (at->aps,
         at->num_aps,
         sizeof (AutomationPoint *),
         cmpfunc);

  if (generate_curve_points)
    {
      /* add midway automation point to control curviness */
      AutomationPoint * prev_ap =
        automation_track_get_prev_ap (at,
                                      ap);
      AutomationPoint * next_ap =
        automation_track_get_next_ap (at,
                                      ap);
      /* 4 cases */
      if (!prev_ap && !next_ap) /* only ap */
        {
          /* do nothing */
        }
      else if (prev_ap && !next_ap) /* last ap */
        {
          create_curve_point_before (at,
                                     prev_ap,
                                     ap);
        }
      else if (!prev_ap && next_ap) /* first ap */
        {
          create_curve_point_after (at,
                                    next_ap,
                                    ap);
        }
      else  /* has next & prev */
        {
          /* remove existing curve ap */
          AutomationCurve * prev_curve =
            automation_track_get_next_curve_ac (at,
                                                prev_ap);
          automation_track_remove_ac (at, prev_curve);

          create_curve_point_before (at,
                                     prev_ap,
                                     ap);
          create_curve_point_after (at,
                                    next_ap,
                                    ap);
        }
    }

  /* refresh indices */
  for (int i = 0;
       i < at->num_aps;
       i++)
    at->aps[i]->index = i;
}

AutomationPoint *
automation_track_get_ap_before_curve (
  AutomationTrack * at,
  AutomationCurve * ac)
{
  if (ac && ac->index < at->num_aps)
    return at->aps[ac->index];

  return NULL;

  /*for (int i = at->num_automation_points - 1; i >= 0; i--)*/
    /*{*/
      /*AutomationPoint * ap = at->automation_points[i];*/
      /*if (position_compare (&ac->pos,*/
                            /*&ap->pos) >= 0)*/
        /*{*/
          /*return ap;*/
        /*}*/
    /*}*/
  /*return NULL;*/
}

/**
 * Returns the ap after the curve point.
 */
AutomationPoint *
automation_track_get_ap_after_curve (
  AutomationTrack * at,
  AutomationCurve * ac)
{
  if (ac &&
      ac->index < at->num_aps - 1)
    return at->aps[ac->index + 1];

  return NULL;

  /*for (int i = 0; i < at->num_automation_points; i++)*/
    /*{*/
      /*AutomationPoint * ap = at->automation_points[i];*/
      /*if (position_compare (&ap->pos,*/
                            /*&ac->pos) >= 0)*/
        /*{*/
          /*return ap;*/
        /*}*/
    /*}*/
  /*return NULL;*/
}

/**
 * Returns the automation point before the position.
 */
AutomationPoint *
automation_track_get_prev_ap (AutomationTrack * at,
                              AutomationPoint * _ap)
{
  /*int index = automation_track_get_ap_index (at, _ap);*/
  /*g_warn_if_fail (index > -1);*/
  /*for (int i = index - 1; i >= 0; i--)*/
    /*{*/
      /*AutomationPoint * ap = at->automation_points[i];*/
      /*if (position_compare (&_ap->pos,*/
                            /*&ap->pos) >= 0)*/
        /*{*/
          /*return ap;*/
        /*}*/
    /*}*/

  if (_ap->index > 0)
    return at->aps[_ap->index - 1];

  return NULL;
}

/**
 * Returns the automation point after the position.
 */
AutomationPoint *
automation_track_get_next_ap (
  AutomationTrack * at,
  AutomationPoint * _ap)
{
  /*int index = automation_track_get_ap_index (at, _ap);*/
  /*g_warn_if_fail (index > -1);*/
  /*for (int i = index + 1; i < at->num_automation_points; i++)*/
    /*{*/
      /*AutomationPoint * ap = at->automation_points[i];*/
      /*if (position_compare (&ap->pos,*/
                            /*&_ap->pos) >= 0)*/
        /*{*/
          /*return ap;*/
        /*}*/
    /*}*/
  if (_ap &&
      _ap->index < at->num_aps - 1)
    return at->aps[_ap->index + 1];

  return NULL;
}

/**
 * Returns the curve point right after the given ap
 */
AutomationCurve *
automation_track_get_next_curve_ac (
  AutomationTrack * at,
  AutomationPoint * ap)
{
  /*int index = automation_track_get_ap_index (at, ap);*/

  /* if not last or only AP */
  if (ap->index != at->num_aps - 1)
    return at->acs[ap->index];

  return NULL;
}

/**
 * Removes automation point from automation track.
 */
void
automation_track_remove_ap (AutomationTrack * at,
                            AutomationPoint * ap)
{
  array_delete (at->aps,
                at->num_aps,
                ap);
  free_later (ap, automation_point_free);
}

/**
 * Removes automation point from automation track.
 */
void
automation_track_remove_ac (AutomationTrack * at,
                            AutomationCurve * ac)
{
  array_delete (at->acs,
                at->num_acs,
                ac);
  free_later (ac, automation_curve_free);
}

/*int*/
/*automation_track_get_ap_index (AutomationTrack * at,*/
                               /*AutomationPoint * ap)*/
/*{*/
  /*for (int i = 0; i < at->num_automation_points; i++)*/
    /*{*/
      /*if (at->automation_points[i] == ap)*/
        /*{*/
          /*return i;*/
        /*}*/
    /*}*/
  /*return -1;*/
/*}*/

/**
 * Returns the automation curve at the given pos.
 */
AutomationCurve *
automation_track_get_ac_at_pos (
  AutomationTrack * self,
  Position *        pos)
{
  AutomationPoint * prev_ap =
    automation_track_get_ap_before_pos (
      self, pos);
  if (!prev_ap)
    return NULL;
  AutomationPoint * next_ap =
    automation_track_get_next_ap (
      self, prev_ap);
  if (!next_ap)
    return NULL;
  return automation_track_get_next_curve_ac (
          self,
          prev_ap);
}

/**
 * Returns the automation point before the pos.
 */
AutomationPoint *
automation_track_get_ap_before_pos (
  AutomationTrack * self,
  Position *        pos)
{
  AutomationPoint * ap;

  /*for (int i = self->num_automation_points - 1;*/
       /*i >= 0;*/
       /*i--)*/
    /*{*/
      /*ap = self->automation_points[i];*/
      /*if (position_compare (&ap->pos,*/
                            /*pos) <= 0)*/
        /*return ap;*/
    /*}*/
  /*return NULL;*/

  /* binary search */
  int first = 0;
  /*g_message ("num autom %d",*/
             /*self->num_automation_points);*/
  int last = self->num_aps - 1;
  if (first == last)
    return NULL;
  int middle = (first+last)/2;
  AutomationPoint * next_ap;
  int ap_is_before, next_ap_is_before;

  while (first <= last)
    {
      ap = self->aps[middle];
      next_ap = NULL;
      next_ap_is_before = 0;
      if (middle < last)
        {
          next_ap =
            self->aps[middle + 1];
          next_ap_is_before =
            position_compare (
              &next_ap->pos, pos) <= 0;
        }
      ap_is_before =
        position_compare (
          &ap->pos, pos) <= 0;

      /* if both too early, look in the 2nd half*/
      if (ap_is_before && (
            next_ap_is_before))
        first = middle + 1;
      else if (ap_is_before)
        return ap;
      else
        last = middle - 1;

      middle = (first + last)/2;
   }
  return NULL;
}

/**
 * Returns the normalized value (0.0-1.0) at the
 * given position.
 *
 * If there is no automation point/curve during
 * the position, it returns negative.
 */
float
automation_track_get_normalized_val_at_pos (
  AutomationTrack * self,
  Position *        pos)
{
  AutomationCurve * ac =
    automation_track_get_ac_at_pos (
      self, pos);
  AutomationPoint * prev_ap =
    automation_track_get_ap_before_pos (
      self, pos);

  /* no automation points yet, return negative
   * (no change) */
  if (!ac && !prev_ap)
    return -1.f;

  AutomationPoint * next_ap =
    automation_track_get_next_ap (
      self, prev_ap);
  /*g_message ("prev fvalue %f next %f",*/
             /*prev_ap->fvalue,*/
             /*next_ap->fvalue);*/
  float prev_ap_normalized_val =
    automation_point_get_normalized_value (
      prev_ap);

  /* return value at last ap */
  if (!next_ap)
    return prev_ap_normalized_val;

  float next_ap_normalized_val =
    automation_point_get_normalized_value (
      next_ap);
  int prev_ap_lower =
    prev_ap_normalized_val <= next_ap_normalized_val;
  float prev_next_diff =
    fabs (prev_ap_normalized_val -
          next_ap_normalized_val);

  /* ratio of how far in we are in the curve */
  int pos_ticks = position_to_ticks (pos);
  int prev_ap_ticks =
    position_to_ticks (&prev_ap->pos);
  int next_ap_ticks =
    position_to_ticks (&next_ap->pos);
  double ratio =
    (double) (pos_ticks - prev_ap_ticks) /
    (next_ap_ticks - prev_ap_ticks);
  /*g_message ("ratio %f",*/
             /*ratio);*/

  double result =
    automation_curve_get_y_normalized (
      ratio, ac->curviness, !prev_ap_lower);
  result = result * prev_next_diff;
  /*g_message ("halfbaked result %f start at lower %d",*/
             /*result, prev_ap_lower);*/
  if (prev_ap_lower)
    result +=
      prev_ap_normalized_val;
  else
    result +=
      next_ap_normalized_val;

  /*g_message ("result of %s: %f",*/
             /*self->automatable->label, result);*/
  return result;
}

/*int*/
/*automation_track_get_curve_index (AutomationTrack * at,*/
                                  /*AutomationCurve * ac)*/
/*{*/
  /*for (int i = 0; i < at->num_automation_curves; i++)*/
    /*{*/
      /*if (at->automation_curves[i] == ac)*/
        /*{*/
          /*return i;*/
        /*}*/
    /*}*/
  /*return -1;*/
/*}*/

void
automation_track_free (AutomationTrack * self)
{
  int i;
  for (i = 0; i < self->num_aps; i++)
    automation_track_remove_ap (
      self, self->aps[i]);

  for (i = 0; i < self->num_acs; i++)
    automation_track_remove_ac (
      self, self->acs[i]);

  free (self);
}
