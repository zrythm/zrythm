/*
 * audio/automation_track.c - Automation track
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_point.h"
#include "audio/track.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/arranger.h"
#include "utils/arrays.h"

AutomationTrack *
automation_track_new (Track *         track,
                      Automatable *   a)
{
  AutomationTrack * at = calloc (1, sizeof (AutomationTrack));

  at->track = track;
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

/**
 * Gets automation track for given automatable, if any.
 */
AutomationTrack *
automation_track_get_for_automatable (Automatable * automatable)
{
  Track * track = automatable->track;

  for (int i = 0; i < track->num_automation_tracks; i++)
    {
      AutomationTrack * at = track->automation_tracks[i];
      if (at->automatable == automatable)
        {
          return at;
        }
    }
  return NULL;
}

static int
cmpfunc (const void * _a, const void * _b)
{
  AutomationPoint * a = *(AutomationPoint **)_a;
  AutomationPoint * b = *(AutomationPoint **)_b;
  int ret = position_compare (&a->pos,
                              &b->pos);
  if (ret == 0 &&
      automation_track_get_index (a->at,
                                  a) <
      automation_track_get_index (a->at,
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
  AutomationPoint * curve = automation_point_new_curve (at, pos);
  automation_track_add_automation_point (at, curve, NO_GENERATE_CURVE_POINTS);

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
      g_message ("%d: type %d", i, ap->type);
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
}

/**
 * Adds automation point and optionally generates curve points accordingly.
 */
void
automation_track_add_automation_point (AutomationTrack * at,
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

  if (ap->type == AUTOMATION_POINT_VALUE && generate_curve_points)
    {
      /* add midway automation point to control curviness */
      AutomationPoint * prev_ap = automation_track_get_prev_ap (at,
                                                                ap);
      AutomationPoint * next_ap = automation_track_get_next_ap (at,
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
          AutomationPoint * prev_curve =
            automation_track_get_next_curve_ap (at,
                                                prev_ap);
          automation_track_remove_ap (at, prev_curve);

          create_curve_point_before (at,
                                     prev_ap,
                                     ap);
          create_curve_point_after (at,
                                    next_ap,
                                    ap);
        }
    }
}

/**
 * Returns the automation point before the position.
 */
AutomationPoint *
automation_track_get_prev_ap (AutomationTrack * at,
                              AutomationPoint * _ap)
{
  int index = automation_track_get_index (at, _ap);
  for (int i = index - 1; i >= 0; i--)
    {
      AutomationPoint * ap = at->automation_points[i];
      if (ap->type == AUTOMATION_POINT_VALUE &&
          position_compare (&_ap->pos,
                            &ap->pos) >= 0)
        {
          return ap;
        }
    }
  return NULL;
}

/**
 * Returns the curve point right after the given ap
 */
AutomationPoint *
automation_track_get_next_curve_ap (AutomationTrack * at, /* FIXME don't need at */
                                    AutomationPoint * _ap)
{
  for (int i = 0; i < at->num_automation_points; i++)
    {
      AutomationPoint * ap = at->automation_points[i];
      if (ap == _ap)
        {
          if (i < (at->num_automation_points - 1) &&
              at->automation_points[i + 1]->type == AUTOMATION_POINT_CURVE)
            {
              return at->automation_points[i + 1];
            }
          else if (i < (at->num_automation_points - 2) &&
                   at->automation_points [i + 2]->type == AUTOMATION_POINT_CURVE)
            {
              return at->automation_points [i + 2];
            }
        }
    }
  g_error ("get next curve ap: no next curve AP point found");
  return NULL;
}

/**
 * Removes automation point from automation track.
 */
void
automation_track_remove_ap (AutomationTrack * at,
                            AutomationPoint * ap)
{
  arrays_delete ((void **) at->automation_points,
                 &at->num_automation_points,
                 ap);
  automation_point_free (ap);
}

/**
 * Returns the automation point after the position.
 */
AutomationPoint *
automation_track_get_next_ap (AutomationTrack * at,
                              AutomationPoint * _ap)
{
  int index = automation_track_get_index (at, _ap);
  for (int i = index + 1; i < at->num_automation_points; i++)
    {
      AutomationPoint * ap = at->automation_points[i];
      if (ap->type == AUTOMATION_POINT_VALUE &&
          position_compare (&ap->pos,
                            &_ap->pos) >= 0)
        {
          return ap;
        }
    }
  return NULL;
}

int
automation_track_get_index (AutomationTrack * at,
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

void
automation_track_free (AutomationTrack * at)
{
  /* FIXME free allocated members too */
  free (at);
}
