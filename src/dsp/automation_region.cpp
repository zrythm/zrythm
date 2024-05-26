// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "dsp/automation_point.h"
#include "dsp/automation_region.h"
#include "dsp/position.h"
#include "dsp/region.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm_app.h"

int
automation_region_sort_func (const void * _a, const void * _b)
{
  AutomationPoint * a = *(AutomationPoint * const *) _a;
  AutomationPoint * b = *(AutomationPoint * const *) _b;
  ArrangerObject *  a_obj = (ArrangerObject *) a;
  ArrangerObject *  b_obj = (ArrangerObject *) b;
  long              ret = position_compare_frames (&a_obj->pos, &b_obj->pos);
  if (ret == 0 && a->index < b->index)
    {
      return -1;
    }

  return (int) CLAMP (ret, -1, 1);
}

ZRegion *
automation_region_new (
  const Position * start_pos,
  const Position * end_pos,
  unsigned int     track_name_hash,
  int              at_idx,
  int              idx_inside_at)
{
  ZRegion * self = object_new (ZRegion);

  self->id.type = RegionType::REGION_TYPE_AUTOMATION;

  self->aps_size = 2;
  self->aps = object_new_n (self->aps_size, AutomationPoint *);

  region_init (self, start_pos, end_pos, track_name_hash, at_idx, idx_inside_at);

  return self;
}

/**
 * Prints the automation in this Region.
 */
void
automation_region_print_automation (ZRegion * self)
{
  AutomationPoint * ap;
  ArrangerObject *  ap_obj;
  for (int i = 0; i < self->num_aps; i++)
    {
      ap = self->aps[i];
      ap_obj = (ArrangerObject *) ap;
      g_message ("%d", i);
      position_print (&ap_obj->pos);
    }
}

/**
 * Forces sort of the automation points.
 */
void
automation_region_force_sort (ZRegion * self)
{
  /* sort by position */
  qsort (
    self->aps, (size_t) self->num_aps, sizeof (AutomationPoint *),
    automation_region_sort_func);

  /* refresh indices */
  for (int i = 0; i < self->num_aps; i++)
    {
      automation_point_set_region_and_index (self->aps[i], self, i);
    }
}

/**
 * Adds an AutomationPoint to the Region.
 */
void
automation_region_add_ap (ZRegion * self, AutomationPoint * ap, int pub_events)
{
  g_return_if_fail (IS_REGION (self) && IS_ARRANGER_OBJECT (ap));

  /* add point */
  array_double_size_if_full (
    self->aps, self->num_aps, self->aps_size, AutomationPoint *);
  array_append (self->aps, self->num_aps, ap);

  /* re-sort */
  automation_region_force_sort (self);

  if (pub_events)
    {
      EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, ap);
    }
}

/**
 * Returns the AutomationPoint before the given
 * one.
 */
AutomationPoint *
automation_region_get_prev_ap (ZRegion * self, AutomationPoint * ap)
{
  if (ap->index > 0)
    return self->aps[ap->index - 1];

  return NULL;
}

/**
 * Returns the AutomationPoint after the given
 * one.
 *
 * @param check_positions Compare positions instead
 *   of just getting the next index.
 * @param check_transients Also check the transient
 *   of each object. This only matters if \ref
 *   check_positions is true. FIXME not used at
 *   the moment. Keep it around for abit then
 *   delete it if not needed.
 */
AutomationPoint *
automation_region_get_next_ap (
  ZRegion *         self,
  AutomationPoint * ap,
  bool              check_positions,
  bool              check_transients)
{
  g_return_val_if_fail (self && ap, NULL);

  if (check_positions)
    {
      check_transients =
        ZRYTHM_HAVE_UI && MW_AUTOMATION_ARRANGER
        && MW_AUTOMATION_ARRANGER->action == UI_OVERLAY_ACTION_MOVING_COPY;
      ArrangerObject *  obj = (ArrangerObject *) ap;
      AutomationPoint * next_ap = NULL;
      ArrangerObject *  next_obj = NULL;
      const int         loop_times = check_transients ? 2 : 1;
      for (int i = 0; i < self->num_aps; i++)
        {
          for (int j = 0; j < loop_times; j++)
            {
              AutomationPoint * cur_ap = self->aps[i];
              ArrangerObject *  cur_obj = (ArrangerObject *) cur_ap;
              if (j == 1)
                {
                  if (cur_obj->transient)
                    {
                      cur_obj = cur_obj->transient;
                      cur_ap = (AutomationPoint *) cur_obj;
                    }
                  else
                    continue;
                }

              if (cur_ap == ap)
                continue;

              if (
                position_is_after_or_equal (&cur_obj->pos, &obj->pos)
                && (!next_obj || position_is_before (&cur_obj->pos, &next_obj->pos)))
                {
                  next_obj = cur_obj;
                  next_ap = cur_ap;
                }
            }
        }
      return next_ap;
    }
  else if (ap->index < self->num_aps - 1)
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
  ZRegion *         self,
  AutomationPoint * ap,
  bool              freeing_region,
  int               free)
{
  g_return_if_fail (IS_REGION (self) && IS_ARRANGER_OBJECT (ap));

  /* deselect */
  arranger_object_select (
    (ArrangerObject *) ap, F_NO_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

  if (self->last_recorded_ap == ap)
    {
      self->last_recorded_ap = NULL;
    }

  array_delete (self->aps, self->num_aps, ap);

  if (!freeing_region)
    {
      for (int i = 0; i < self->num_aps; i++)
        {
          automation_point_set_region_and_index (self->aps[i], self, i);
        }
    }

  if (free)
    {
      /* FIXME this was the previous comment, it's now free'd
       * immediately so there may be issues */
      /* free later otherwise causes problems while recording */
      /*free_later (ap, arranger_object_free);*/
      arranger_object_free ((ArrangerObject *) ap);
    }
}

/**
 * Returns the automation points since the last
 * recorded automation point (if the last recorded
 * automation point was before the current pos).
 */
void
automation_region_get_aps_since_last_recorded (
  ZRegion *   self,
  Position *  pos,
  GPtrArray * aps)
{
  g_ptr_array_remove_range (aps, 0, aps->len);

  ArrangerObject * last_recorded_obj = (ArrangerObject *) self->last_recorded_ap;
  if (
    !last_recorded_obj
    || position_is_before_or_equal (pos, &last_recorded_obj->pos))
    return;

  for (int i = 0; i < self->num_aps; i++)
    {
      AutomationPoint * ap = self->aps[i];
      ArrangerObject *  ap_obj = (ArrangerObject *) ap;

      if (
        position_is_after (&ap_obj->pos, &last_recorded_obj->pos)
        && position_is_before_or_equal (&ap_obj->pos, pos))
        {
          g_ptr_array_add (aps, ap);
        }
    }
}

/**
 * Returns an automation point found within +/-
 * delta_ticks from the position, or NULL.
 *
 * @param before_only Only check previous automation
 *   points.
 */
AutomationPoint *
automation_region_get_ap_around (
  ZRegion *  self,
  Position * _pos,
  double     delta_ticks,
  bool       before_only,
  bool       use_snapshots)
{
  Position pos;
  position_set_to_pos (&pos, _pos);
  AutomationTrack * at = region_get_automation_track (self);
  /* FIXME only check aps in this region */
  AutomationPoint * ap =
    automation_track_get_ap_before_pos (at, &pos, true, use_snapshots);
  ArrangerObject * ap_obj = (ArrangerObject *) ap;
  if (ap && pos.ticks - ap_obj->pos.ticks <= (double) delta_ticks)
    {
      return ap;
    }
  else if (!before_only)
    {
      position_add_ticks (&pos, delta_ticks);
      ap = automation_track_get_ap_before_pos (at, &pos, true, use_snapshots);
      ap_obj = (ArrangerObject *) ap;
      if (ap)
        {
          double diff = ap_obj->pos.ticks - _pos->ticks;
          if (diff >= 0.0)
            return ap;
        }
    }

  return NULL;
}

bool
automation_region_validate (ZRegion * self)
{
  for (int i = 0; i < self->num_aps; i++)
    {
      AutomationPoint * ap = self->aps[i];

      g_return_val_if_fail (ap->index == i, false);
    }

  return true;
}

/**
 * Frees members only but not the ZRegion itself.
 *
 * Regions should be free'd using region_free.
 */
void
automation_region_free_members (ZRegion * self)
{
  int i;
  for (i = self->num_aps - 1; i >= 0; i--)
    {
      automation_region_remove_ap (self, self->aps[i], true, F_FREE);
    }

  free (self->aps);
}
