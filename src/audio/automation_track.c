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
#include "gui/widgets/automation_track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/arrays.h"

AutomationTrack *
automation_track_new (Automatable *   a)
{
  AutomationTrack * at = calloc (1, sizeof (AutomationTrack));

  at->track = a->track;
  automation_track_set_automatable (at, a);

  return at;
}

/**
 * Sets the automatable to the automation track and updates the GUI
 */
void
automation_track_set_automatable (AutomationTrack * automation_track,
                                  Automatable *     a)
{
  automation_track->automatable = a;

  /* TODO update GUI */

}

/**
 * Updates automation track & its GUI
 */
void
automation_track_update (AutomationTrack * at)
{
  automation_track_widget_update (at->widget);
}


static int
cmpfunc (const void * _a, const void * _b)
{
  AutomationPoint * a = *(AutomationPoint **)_a;
  AutomationPoint * b = *(AutomationPoint **)_b;
  int ret = position_compare (&a->pos,
                              &b->pos);
  if (ret == 0 &&
      automation_track_get_ap_index (a->at,
                                  a) <
      automation_track_get_ap_index (a->at,
                                  b))
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
      automation_track_get_curve_index (a->at,
                                        a) <
      automation_track_get_curve_index (a->at,
                                        b))
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
  AutomationCurve * curve = automation_curve_new (at, pos);
  automation_track_add_automation_curve (at, curve);

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
  g_assert (position_compare (&mid_pos,
                              &ap->pos) <= 0);
  g_assert (position_compare (&mid_pos,
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
  for (int i = 0; i < at->num_automation_points; i++)
    {
      AutomationPoint * ap = at->automation_points[i];
      g_message ("%d", i);
      position_print (&ap->pos);
    }
}

void
automation_track_force_sort (AutomationTrack * at)
{
  /* sort by position */
  qsort (at->automation_points,
         at->num_automation_points,
         sizeof (AutomationPoint *),
         cmpfunc);
  qsort (at->automation_curves,
         at->num_automation_curves,
         sizeof (AutomationCurve *),
         cmpfunc_curve);
}

/**
 * Adds automation curve.
 */
void
automation_track_add_automation_curve (AutomationTrack * at,
                                       AutomationCurve * ac)
{
  /* add point */
  array_append (at->automation_curves,
                at->num_automation_curves,
                ac);

  /* sort by position */
  qsort (at->automation_curves,
         at->num_automation_curves,
         sizeof (AutomationCurve *),
         cmpfunc_curve);
}

/**
 * Adds automation point and optionally generates curve points accordingly.
 */
void
automation_track_add_automation_point (
  AutomationTrack * at,
  AutomationPoint * ap,
  int               generate_curve_points)
{
  /* add point */
  at->automation_points[at->num_automation_points++] = ap;

  /* sort by position */
  qsort (at->automation_points,
         at->num_automation_points,
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
}

AutomationPoint *
automation_track_get_ap_before_curve (AutomationTrack * at,
                                      AutomationCurve * ac)
{
  for (int i = at->num_automation_points - 1; i >= 0; i--)
    {
      AutomationPoint * ap = at->automation_points[i];
      if (position_compare (&ac->pos,
                            &ap->pos) >= 0)
        {
          return ap;
        }
    }
  return NULL;
}

/**
 * Returns the ap after the curve point.
 */
AutomationPoint *
automation_track_get_ap_after_curve (AutomationTrack * at,
                                     AutomationCurve * ac)
{
  for (int i = 0; i < at->num_automation_points; i++)
    {
      AutomationPoint * ap = at->automation_points[i];
      if (position_compare (&ap->pos,
                            &ac->pos) >= 0)
        {
          return ap;
        }
    }
  return NULL;
}

/**
 * Returns the automation point before the position.
 */
AutomationPoint *
automation_track_get_prev_ap (AutomationTrack * at,
                              AutomationPoint * _ap)
{
  int index = automation_track_get_ap_index (at, _ap);
  g_assert (index > -1);
  for (int i = index - 1; i >= 0; i--)
    {
      AutomationPoint * ap = at->automation_points[i];
      if (position_compare (&_ap->pos,
                            &ap->pos) >= 0)
        {
          return ap;
        }
    }
  return NULL;
}

/**
 * Returns the automation point after the position.
 */
AutomationPoint *
automation_track_get_next_ap (AutomationTrack * at,
                              AutomationPoint * _ap)
{
  int index = automation_track_get_ap_index (at, _ap);
  g_assert (index > -1);
  for (int i = index + 1; i < at->num_automation_points; i++)
    {
      AutomationPoint * ap = at->automation_points[i];
      if (position_compare (&ap->pos,
                            &_ap->pos) >= 0)
        {
          return ap;
        }
    }
  return NULL;
}

/**
 * Returns the curve point right after the given ap
 */
AutomationCurve *
automation_track_get_next_curve_ac (AutomationTrack * at,
                                    AutomationPoint * ap)
{
  int index = automation_track_get_ap_index (at, ap);

  /* if not last or only AP */
  if (!(index == at->num_automation_points - 1))
    return at->automation_curves[index];

  return NULL;
}

/**
 * Removes automation point from automation track.
 */
void
automation_track_remove_ap (AutomationTrack * at,
                            AutomationPoint * ap)
{
  array_delete (at->automation_points,
                at->num_automation_points,
                ap);
  automation_point_free (ap);
}

/**
 * Removes automation point from automation track.
 */
void
automation_track_remove_ac (AutomationTrack * at,
                            AutomationCurve * ac)
{
  array_delete (at->automation_curves,
                at->num_automation_curves,
                ac);
  automation_curve_free (ac);
}


int
automation_track_get_ap_index (AutomationTrack * at,
                               AutomationPoint * ap)
{
  for (int i = 0; i < at->num_automation_points; i++)
    {
      if (at->automation_points[i] == ap)
        {
          return i;
        }
    }
  return -1;
}

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
  for (int i = self->num_automation_points - 1;
       i >= 0;
       i--)
    {
      ap = self->automation_points[i];
      if (position_compare (&ap->pos,
                            pos) <= 0)
        return ap;
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
automation_track_get_value_at_pos (
  AutomationTrack * self,
  Position *        pos)
{
  AutomationCurve * ac =
    automation_track_get_ac_at_pos (
      self, pos);
  if (!ac)
    return -1.f;
  AutomationPoint * prev_ap =
    automation_track_get_ap_before_pos (
      self, pos);
  AutomationPoint * next_ap =
    automation_track_get_next_ap (
      self, prev_ap);
  /*g_message ("prev fvalue %f next %f",*/
             /*prev_ap->fvalue,*/
             /*next_ap->fvalue);*/
  int prev_ap_lower =
    prev_ap->fvalue <= next_ap->fvalue;
  float prev_next_diff = fabs (prev_ap->fvalue - next_ap->fvalue);

  /* ratio of how far in we are in the curve */
  int pos_ticks = position_to_ticks (pos);
  int prev_ap_ticks = position_to_ticks (&prev_ap->pos);
  int next_ap_ticks = position_to_ticks (&next_ap->pos);
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
  result += prev_ap->fvalue;

  g_message ("result %f",
             result);
  return result;
}

int
automation_track_get_curve_index (AutomationTrack * at,
                                  AutomationCurve * ac)
{
  for (int i = 0; i < at->num_automation_curves; i++)
    {
      if (at->automation_curves[i] == ac)
        {
          return i;
        }
    }
  return -1;
}

void
automation_track_free (AutomationTrack * at)
{
  /* FIXME free allocated members too */
  free (at);
}
