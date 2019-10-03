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
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/center_dock.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"

#define SET_POS(_c,pos_name,_pos,_trans_only) \
  ARRANGER_OBJ_SET_POS ( \
    automation_point, _c, pos_name, _pos, \
    _trans_only)

ARRANGER_OBJ_DEFINE_MOVABLE (
  AutomationPoint, automation_point,
  automation_selections, AUTOMATION_SELECTIONS);

static AutomationPoint *
_create_new (
  const Position *        pos)
{
  AutomationPoint * ap =
    calloc (1, sizeof (AutomationPoint));

  position_set_to_pos (
    &ap->pos, pos);
  ap->index = -1;

  return ap;
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

  for (int i = 0; i < 2; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        ap =
  automation_point_get_main_automation_point (
    ap);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        ap =
  automation_point_get_main_trans_automation_point (
    ap);

      ap->region = region;
      ap->region_name = g_strdup (region->name);
      ap->index = index;
    }
}

void
automation_point_init_loaded (
  AutomationPoint * self)
{
  ARRANGER_OBJECT_SET_AS_MAIN (
    AUTOMATION_POINT, AutomationPoint,
    automation_point);

  automation_point_set_region_and_index (
    self, self->region, self->index);
}

static int
automation_point_is_equal (
  AutomationPoint * a,
  AutomationPoint * b)
{
  return
    position_is_equal (&a->pos, &b->pos) &&
    math_floats_equal (
      a->fvalue, b->fvalue, 0.001f);
}

/**
 * Returns the Track this AutomationPoint is in.
 */
Track *
automation_point_get_track (
  AutomationPoint * ap)
{
  return ap->region->at->track;
}

/**
 * Finds the automation point in the project matching
 * the params of the given one.
 */
AutomationPoint *
automation_point_find (
  AutomationPoint * src)
{
  g_warn_if_fail (src->region);

  /* the src region might be an unused clone, find
   * the actual region. */
  Region * region =
    region_find (src->region);

  int i;
  AutomationPoint * ap;
  for (i = 0; i < region->num_aps; i++)
    {
      ap = region->aps[i];
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

/**
 * Updates the frames of each position in each child
 * of the AutomationPoint recursively.
 */
void
automation_point_update_frames (
  AutomationPoint * self)
{
  position_update_frames (&self->pos);
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

  position_set_to_pos (
    &ap->pos, &src->pos);

  return ap;
}

/**
 * Returns the normalized value (0.0 to 1.0).
 */
float
automation_point_get_normalized_value (
  AutomationPoint * self)
{
  g_warn_if_fail (self->region->at);

  /* TODO convert to macro */
  return automatable_real_val_to_normalized (
    self->region->at->automatable,
    self->fvalue);
}

ARRANGER_OBJ_DECLARE_RESET_COUNTERPART (
  AutomationPoint, automation_point)
{
  AutomationPoint * src =
    reset_trans ?
      automation_point_get_main_automation_point (automation_point) :
      automation_point_get_main_trans_automation_point (automation_point);
  AutomationPoint * dest =
    reset_trans ?
      automation_point_get_main_trans_automation_point (automation_point) :
      automation_point_get_main_automation_point (automation_point);

  position_set_to_pos (
    &dest->pos, &src->pos);
  dest->fvalue = src->fvalue;
}

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
 * Updates the value from given real value and
 * notifies interested parties.
 */
void
automation_point_update_fvalue (
  AutomationPoint * self,
  float             real_val,
  ArrangerObjectUpdateFlag update_flag)
{
  ARRANGER_OBJ_SET_PRIMITIVE_VAL (
    AutomationPoint, self, fvalue, real_val,
    update_flag);

  g_return_if_fail (self->region);
  Automatable * a = self->region->at->automatable;
  automatable_set_val_from_normalized (
    a,
    automatable_real_val_to_normalized (
      a, real_val), 1);

  EVENTS_PUSH (ET_AUTOMATION_POINT_CHANGED, self);
}

/**
 * Destroys the widget and frees memory.
 */
void
automation_point_free (
  AutomationPoint * self)
{
  if (GTK_IS_WIDGET (self->widget))
    gtk_widget_destroy (GTK_WIDGET (self->widget));

  if (self->region_name)
    g_free (self->region_name);

  free (self);
}
