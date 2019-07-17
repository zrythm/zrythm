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

/**
 * \file
 *
 * API for automation Region's.
 */

#ifndef __AUDIO_AUTOMATION_REGION_H__
#define __AUDIO_AUTOMATION_REGION_H__

typedef struct Track Track;
typedef struct Position Position;
typedef struct AutomationPoint AutomationPoint;
typedef struct AutomationCurve AutomationCurve;
typedef struct Region Region;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Creates a new Region for automation.
 *
 * @param is_main If this is 1 it
 *   will create the additional Region (
 *   main_transient).
 */
Region *
automation_region_new (
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main);

/**
 * Prints the automation in this Region.
 */
void
automation_region_print_automation (
  Region * self);

/**
 * Forces sort of the automation points.
 */
void
automation_region_force_sort (
  Region * self);

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
  int               gen_curves);

/**
 * Adds the given AutomationCurve.
 */
void
automation_region_add_ac (
  Region *          self,
  AutomationCurve * ac);

/**
 * Returns the AutomationPoint before the position.
 */
AutomationPoint *
automation_region_get_prev_ap (
  Region *          self,
  AutomationPoint * ap);

/**
 * Returns the AutomationPoint after the position.
 */
AutomationPoint *
automation_region_get_next_ap (
  Region *          self,
  AutomationPoint * ap);

/**
 * Returns the AutomationPoint before the given
 * AutomationCurve.
 */
AutomationPoint *
automation_region_get_ap_before_curve (
  Region *          self,
  AutomationCurve * ac);

/**
 * Returns the ap after the curve point.
 */
AutomationPoint *
automation_region_get_ap_after_curve (
  Region *          self,
  AutomationCurve * ac);

/**
 * Returns the curve point right after the given ap
 */
AutomationCurve *
automation_region_get_next_curve_ac (
  Region *          self,
  AutomationPoint * ap);

/**
 * Removes the given AutomationPoint from the Region.
 *
 * @param free Optionally free the AutomationPoint.
 */
void
automation_region_remove_ap (
  Region *          self,
  AutomationPoint * ap,
  int               free);

/**
 * Removes the given AutomationCurve from the Region.
 *
 * @param free Optionally free the AutomationCurve.
 */
void
automation_region_remove_ac (
  Region *          self,
  AutomationCurve * ac,
  int               free);

/**
 * Frees members only but not the Region itself.
 *
 * Regions should be free'd using region_free.
 */
void
automation_region_free_members (
  Region * self);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_REGION_H__
