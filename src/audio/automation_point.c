/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 */

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_point.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/port.h"
#include "audio/position.h"
#include "audio/track.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/center_dock.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"

static AutomationPointCurveAlgorithm CURVE_ALGO =
  AP_CURVE_ALGORITHM_SUPERELLIPSE;

static AutomationPoint *
_create_new (
  const Position *        pos)
{
  AutomationPoint * self =
    calloc (1, sizeof (AutomationPoint));

  ArrangerObject * obj =
    (ArrangerObject *) self;
  obj->pos = *pos;
  obj->type = ARRANGER_OBJECT_TYPE_AUTOMATION_POINT;
  self->curviness = 1.0;

  self->index = -1;

  return self;
}

/**
 * Sets the Region and the index in the
 * region that the AutomationPoint
 * belongs to, in all its counterparts.
 */
void
automation_point_set_region_and_index (
  AutomationPoint * ap,
  Region *          region,
  int               index)
{
  g_return_if_fail (ap && region);
  ap->region = region;
  ap->region_name = g_strdup (region->name);
  ap->index = index;
}

int
automation_point_is_equal (
  AutomationPoint * a,
  AutomationPoint * b)
{
  ArrangerObject * a_obj =
    (ArrangerObject *) a;
  ArrangerObject * b_obj =
    (ArrangerObject *) b;
  return
    position_is_equal (&a_obj->pos, &b_obj->pos) &&
    MATH_FLOATS_EQUAL (
      a->fvalue, b->fvalue, 0.001f);
}

/**
 * Creates an AutomationPoint in the given
 * AutomationTrack at the given Position.
 */
AutomationPoint *
automation_point_new_float (
  const float         value,
  const float         normalized_val,
  const Position *    pos,
  int                 is_main)
{
  AutomationPoint * self =
    _create_new (pos);

  self->fvalue = value;
  self->normalized_val = normalized_val;

  return self;
}

/**
 * Returns the normalized value (0.0 to 1.0).
 */
/*float*/
/*automation_point_get_normalized_value (*/
  /*AutomationPoint * self)*/
/*{*/
  /*g_warn_if_fail (self->region->at);*/

  /*[> TODO convert to macro <]*/
  /*return automatable_real_val_to_normalized (*/
    /*self->region->at->automatable,*/
    /*self->fvalue);*/
/*}*/

/**
 * Moves the AutomationPoint by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only move transients.
 * @return Whether moved or not.
 * FIXME always call this after move !!!!!!!!!
 */
/*void*/
/*automation_point_on_move (*/
  /*AutomationPoint * automation_point,*/
  /*long     ticks,*/
  /*int      use_cached_pos,*/
  /*int      trans_only)*/
/*{*/
  /*Position tmp;*/
  /*POSITION_MOVE_BY_TICKS (*/
    /*tmp, use_cached_pos, automation_point, pos,*/
    /*ticks, trans_only);*/

  /*AutomationPoint * ap = automation_point;*/

  /*[> get prev and next value APs <]*/
  /*AutomationPoint * prev_ap =*/
    /*automation_region_get_prev_ap (*/
      /*ap->region, ap);*/
  /*AutomationPoint * next_ap =*/
    /*automation_region_get_next_ap (*/
      /*ap->region, ap);*/

  /*[> get adjusted pos for this automation point <]*/
  /*Position ap_pos;*/
  /*Position * prev_pos = &ap->cache_pos;*/
  /*position_set_to_pos (&ap_pos,*/
                       /*prev_pos);*/
  /*position_add_ticks (&ap_pos, ticks);*/

  /*Position mid_pos;*/
  /*AutomationCurve * ac;*/

  /*[> update midway points <]*/
  /*if (prev_ap &&*/
      /*position_is_after_or_equal (*/
        /*&ap_pos, &prev_ap->pos))*/
    /*{*/
      /*[> set prev curve point to new midway pos <]*/
      /*position_get_midway_pos (*/
        /*&prev_ap->pos, &ap_pos, &mid_pos);*/
      /*ac =*/
        /*automation_region_get_next_curve_ac (*/
          /*ap->region, prev_ap);*/
      /*position_set_to_pos (&ac->pos, &mid_pos);*/

      /*[> set pos for ap <]*/
      /*if (!next_ap)*/
        /*{*/
          /*position_set_to_pos (&ap->pos, &ap_pos);*/
        /*}*/
    /*}*/
  /*if (next_ap &&*/
      /*position_is_before_or_equal (*/
        /*&ap_pos, &next_ap->pos))*/
    /*{*/
      /*[> set next curve point to new midway pos <]*/
      /*position_get_midway_pos (*/
        /*&ap_pos, &next_ap->pos, &mid_pos);*/
      /*ac =*/
        /*automation_region_get_next_curve_ac (*/
          /*ap->region, ap);*/
      /*position_set_to_pos (&ac->pos, &mid_pos);*/

      /* set pos for ap - if no prev ap exists
       * or if the position is also after the
       * prev ap */
      /*if ((prev_ap &&*/
           /*position_is_after_or_equal (*/
            /*&ap_pos, &prev_ap->pos)) ||*/
          /*(!prev_ap))*/
        /*{*/
          /*position_set_to_pos (&ap->pos, &ap_pos);*/
        /*}*/
    /*}*/
  /*else if (!prev_ap && !next_ap)*/
    /*{*/
      /*[> set pos for ap <]*/
      /*position_set_to_pos (&ap->pos, &ap_pos);*/
    /*}*/
/*}*/

/**
 * Returns if the curve of the AutomationPoint
 * curves upwards as you move right on the x axis.
 */
int
automation_point_curves_up (
  AutomationPoint * self)
{
  g_return_val_if_fail (self, -1);

  AutomationPoint * next_ap =
    automation_region_get_next_ap (
      self->region, self);

  if (!next_ap)
    return 0;

  if (next_ap->fvalue > self->fvalue)
    return 1;
  else
    return 0;
}

/**
 * Sets the value from given real value and
 * notifies interested parties.
 */
void
automation_point_set_fvalue (
  AutomationPoint * self,
  float             real_val)
{
  Automatable * a = self->region->at->automatable;
  float normalized_val =
    automatable_real_val_to_normalized (
      a, real_val);
  self->normalized_val = normalized_val;
  self->fvalue = real_val;

  g_return_if_fail (self->region);
  automatable_set_val_from_normalized (
    a, normalized_val, 1);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CHANGED, self);
}

/**
 * TODO add description.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 * @param x X-coordinate.
 * @param curviness Curviness variable.
 * @param start_higher Start at higher point.
 */
static double
get_y_normalized (
  double x,
  double curviness,
  int    start_higher,
  int    curve_up)
{
  if (!start_higher)
    x = 1.0 - x;
  if (curve_up)
    x = 1.0 - x;

  double val;
  switch (CURVE_ALGO)
    {
    case AP_CURVE_ALGORITHM_EXPONENT:
      val =
        pow (x, curviness);
      if (curve_up)
        val = 1.0 - val;
      return val;
    case AP_CURVE_ALGORITHM_SUPERELLIPSE:
      val =
        pow (
          1.0 - pow (x, curviness),
          (1.0 / curviness));
      if (curve_up)
        val = 1.0 - val;
      return val;
    }

  g_return_val_if_reached (-1.0);
}

/**
 * The function to return a point on the curve.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 *
 * @param ap The start point (0, 0).
 * @param x Normalized x.
 */
double
automation_point_get_normalized_value_in_curve (
  AutomationPoint * self,
  double            x)
{
  AutomationPoint * next_ap =
    automation_region_get_next_ap (
      self->region, self);

  double dy;

  int start_higher =
    next_ap->normalized_val < self->normalized_val;
  dy =
    get_y_normalized (
      x, self->curviness, start_higher,
      self->curve_up);
  return dy;
}

/**
 * Sets the curviness of the AutomationPoint.
 */
void
automation_point_set_curviness (
  AutomationPoint * self,
  const curviness_t curviness,
  const int         curve_up)
{
  if (math_doubles_equal (
        self->curviness, curviness, 0.001))
    return;

  self->curviness = curviness;
  self->curve_up = curve_up;;
  /*ArrangerObject * obj = (ArrangerObject *) self;*/
  /*if (Z_IS_ARRANGER_OBJECT_WIDGET (obj->widget))*/
    /*{*/
      /*arranger_object_widget_force_redraw (*/
        /*Z_ARRANGER_OBJECT_WIDGET (obj->widget));*/
    /*}*/
}

/**
 * Convenience function to return the Automatable
 * that this AutomationPoint is for.
 */
Automatable *
automation_point_get_automatable (
  AutomationPoint * self)
{
  AutomationTrack * at =
    automation_point_get_automation_track (self);
  g_return_val_if_fail (at, NULL);

  return at->automatable;
}

/**
 * Returns Y in pixels from the value based on
 * the given height of the parent.
 */
int
automation_point_get_y (
  AutomationPoint * self,
  int               height)
{
  /* ratio of current value in the range */
  float ap_ratio = self->normalized_val;

  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  int point =
    allocated_h -
    (int) (ap_ratio * (float) allocated_h);
  return point;
}

/**
 * Convenience function to return the
 * AutomationTrack that this AutomationPoint is in.
 */
AutomationTrack *
automation_point_get_automation_track (
  AutomationPoint * self)
{
  g_return_val_if_fail (
    self && self->region, NULL);
  return region_get_automation_track (self->region);
}
