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

/** \file
 */

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_lane.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/port.h"
#include "audio/position.h"
#include "audio/track.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_curve.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"

#define SET_POS(_c,pos_name,_pos,_trans_only) \
  ARRANGER_OBJ_SET_POS ( \
    automation_point, _c, pos_name, _pos, \
    _trans_only)

static AutomationPoint *
_create_new (
  const Position *        pos)
{
  AutomationPoint * ap =
    calloc (1, sizeof (AutomationPoint));

  position_set_to_pos (
    &ap->pos, pos);
  ap->track_pos = -1;
  ap->index = -1;
  ap->at_index = -1;

  return ap;
}

/**
 * Sets the AutomationTrack and the index in the
 * AutomationTrack that the AutomationPoint
 * belongs to, in all its counterparts.
 */
void
automation_point_set_automation_track_and_index (
  AutomationPoint * _ap,
  AutomationTrack * at,
  int               index)
{
  g_return_if_fail (at);

  AutomationPoint * ap;
  for (int i = 0; i < 4; i++)
    {
      if (i == 0)
        ap = automation_point_get_main_automation_point (_ap);
      else if (i == 1)
        ap = automation_point_get_main_trans_automation_point (_ap);
      else if (i == 2)
        ap =
          (AutomationPoint *)
          _ap->obj_info.lane;
      else if (i == 3)
        ap =
          (AutomationPoint *)
          _ap->obj_info.lane_trans;

      ap->at = at;
      ap->at_index = at->index;
      ap->index = index;
      ap->track_pos = at->track_pos;
    }
}

void
automation_point_init_loaded (
  AutomationPoint * ap)
{
  /* TODO */
  /*ap->at =*/
    /*project_get_automation_track (ap->at_id);*/
}

int
automation_point_is_equal (
  AutomationPoint * a,
  AutomationPoint * b)
{
  return
    position_is_equal (&a->pos, &b->pos) &&
    a->fvalue == b->fvalue;
}

/**
 * Finds the automation point in the project matching
 * the params of the given one.
 */
AutomationPoint *
automation_point_find (
  AutomationPoint * src)
{
  g_warn_if_fail (
    src->track_pos > -1 &&
    src->at_index > -1 &&
    src->index > -1);
  Track * track =
    TRACKLIST->tracks[src->track_pos];
  AutomationTrack * at =
    track->automation_tracklist.ats[src->at_index];
  g_warn_if_fail (track && at);

  int i;
  AutomationPoint * ap;
  for (i = 0; i < at->num_aps; i++)
    {
      ap = at->aps[i];
      if (automation_point_is_equal (src, ap))
        return ap;
    }

  return NULL;
}

/**
 * Creates an AutomationPoint in the given
 * AutomationTrack at the given Position.
 */
AutomationPoint *
automation_point_new_float (
  const float         value,
  const Position *    pos,
  int                 is_main)
{
  AutomationPoint * self =
    _create_new (pos);

  self->fvalue = value;

  if (is_main)
    {
      ARRANGER_OBJECT_SET_AS_MAIN (
        AUTOMATION_POINT, AutomationPoint,
        automation_point);
    }

  return self;
}

ARRANGER_OBJ_DEFINE_GEN_WIDGET_LANELESS (
  AutomationPoint, automation_point);

/**
 * Clones the atuomation point.
 */
AutomationPoint *
automation_point_clone (
  AutomationPoint * src,
  AutomationPointCloneFlag flag)
{
  int is_main = 0;
  if (flag == AUTOMATION_POINT_CLONE_COPY_MAIN)
    is_main = 1;

  AutomationPoint * ap =
    automation_point_new_float (
      src->fvalue, &src->pos, is_main);

  if (src->at)
    automation_point_set_automation_track_and_index (
      ap, src->at, src->index);

  position_set_to_pos (
    &ap->pos, &src->pos);

  return ap;
}

/**
 * Returns Y in pixels from the value based on the
 * allocation of the automation track.
 */
int
automation_point_get_y_in_px (
  AutomationPoint * self)
{
  /* ratio of current value in the range */
  float ap_ratio =
    automation_point_get_normalized_value (self);

  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self->at->al->widget));
  int point = allocated_h - ap_ratio * allocated_h;
  return point;
}

/**
 * Returns the normalized value (0.0 to 1.0).
 */
float
automation_point_get_normalized_value (
  AutomationPoint * self)
{
  g_warn_if_fail (self->at);

  /* TODO convert to macro */
  return automatable_real_val_to_normalized (
    self->at->automatable,
    self->fvalue);
}

DEFINE_IS_ARRANGER_OBJ_SELECTED (
  AutomationPoint, automation_point,
  timeline_selections,
  TL_SELECTIONS);

/**
 * Returns the Track this AutomationPoint is in.
 */
Track *
automation_point_get_track (
  AutomationPoint * ap)
{
  return TRACKLIST->tracks[ap->track_pos];
}

/**
 * Moves the AutomationPoint by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only move transients.
 * @return Whether moved or not.
 */
int
automation_point_move (
  AutomationPoint * automation_point,
  long     ticks,
  int      use_cached_pos,
  int      trans_only)
{
  Position tmp;
  int moved;
  POSITION_MOVE_BY_TICKS (
    tmp, use_cached_pos, automation_point, pos,
    ticks, moved, trans_only);

  AutomationPoint * ap = automation_point;

  /* FIXME */
  /* get prev and next value APs */
  AutomationPoint * prev_ap =
    automation_track_get_prev_ap (ap->at, ap);
  AutomationPoint * next_ap =
    automation_track_get_next_ap (ap->at, ap);

  /* get adjusted pos for this automation point */
  Position ap_pos;
  Position * prev_pos = &ap->cache_pos;
  position_set_to_pos (&ap_pos,
                       prev_pos);
  position_add_ticks (&ap_pos, ticks);

  Position mid_pos;
  AutomationCurve * ac;

  /* update midway points */
  if (prev_ap &&
      position_is_after_or_equal (
        &ap_pos, &prev_ap->pos))
    {
      /* set prev curve point to new midway pos */
      position_get_midway_pos (
        &prev_ap->pos, &ap_pos, &mid_pos);
      ac =
        automation_track_get_next_curve_ac (
          ap->at, prev_ap);
      position_set_to_pos (&ac->pos, &mid_pos);

      /* set pos for ap */
      if (!next_ap)
        {
          position_set_to_pos (&ap->pos, &ap_pos);
        }
    }
  if (next_ap &&
      position_is_before_or_equal (
        &ap_pos, &next_ap->pos))
    {
      /* set next curve point to new midway pos */
      position_get_midway_pos (
        &ap_pos, &next_ap->pos, &mid_pos);
      ac =
        automation_track_get_next_curve_ac (
          ap->at, ap);
      position_set_to_pos (&ac->pos, &mid_pos);

      /* set pos for ap - if no prev ap exists
       * or if the position is also after the
       * prev ap */
      if ((prev_ap &&
           position_is_after_or_equal (
            &ap_pos, &prev_ap->pos)) ||
          (!prev_ap))
        {
          position_set_to_pos (&ap->pos, &ap_pos);
        }
    }
  else if (!prev_ap && !next_ap)
    {
      /* set pos for ap */
      position_set_to_pos (&ap->pos, &ap_pos);
    }

  return moved;
}

ARRANGER_OBJ_DEFINE_SHIFT_TICKS (
  AutomationPoint, automation_point);

/**
 * Updates the value from given real value and
 * notifies interested parties.
 */
void
automation_point_update_fvalue (
  AutomationPoint * self,
  float             real_val,
  int               trans_only)
{
  if (!trans_only)
    {
      automation_point_get_main_automation_point (
        self)->fvalue = real_val;
    }
  automation_point_get_main_trans_automation_point (
    self)->fvalue = real_val;

  Automatable * a = self->at->automatable;
  automatable_set_val_from_normalized (
    a,
    automatable_real_val_to_normalized (
      a, real_val));
  AutomationCurve * ac =
    self->at->acs[self->index];
  if (ac && ac->widget)
    ac->widget->cache = 0;
}

DEFINE_ARRANGER_OBJ_SET_POS (
  AutomationPoint, automation_point);

/**
 * Destroys the widget and frees memory.
 */
void
automation_point_free (AutomationPoint * ap)
{
  if (ap->widget)
    gtk_widget_destroy (GTK_WIDGET (ap->widget));
  free (ap);
}
