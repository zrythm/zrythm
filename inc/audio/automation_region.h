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

#include "audio/region.h"

typedef struct Track Track;
typedef struct Position Position;
typedef struct AutomationPoint AutomationPoint;
typedef struct AutomationCurve AutomationCurve;
typedef struct ZRegion ZRegion;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Creates a new ZRegion for automation.
 *
 * @param is_main If this is 1 it
 *   will create the additional ZRegion (
 *   main_transient).
 */
ZRegion *
automation_region_new (
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main);

/**
 * Prints the automation in this Region.
 */
void
automation_region_print_automation (
  ZRegion * self);

/**
 * Forces sort of the automation points.
 */
void
automation_region_force_sort (
  ZRegion * self);

/**
 * Adds an AutomationPoint to the Region.
 */
void
automation_region_add_ap (
  ZRegion *          self,
  AutomationPoint * ap);

/**
 * Returns the AutomationPoint before the given
 * one.
 */
AutomationPoint *
automation_region_get_prev_ap (
  ZRegion *          self,
  AutomationPoint * ap);

/**
 * Returns the AutomationPoint after the given
 * one.
 */
AutomationPoint *
automation_region_get_next_ap (
  ZRegion *          self,
  AutomationPoint * ap);

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
  int               free);

/**
 * Frees members only but not the ZRegion itself.
 *
 * Regions should be free'd using region_free.
 */
void
automation_region_free_members (
  ZRegion * self);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_REGION_H__
