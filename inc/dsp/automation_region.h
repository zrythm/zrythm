// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * API for automation Region's.
 */

#ifndef __AUDIO_AUTOMATION_REGION_H__
#define __AUDIO_AUTOMATION_REGION_H__

#include "dsp/region.h"

typedef struct Track           Track;
typedef struct Position        Position;
typedef struct AutomationPoint AutomationPoint;
typedef struct AutomationCurve AutomationCurve;
typedef struct ZRegion         ZRegion;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Creates a new ZRegion for automation.
 */
ZRegion *
automation_region_new (
  const Position * start_pos,
  const Position * end_pos,
  unsigned int     track_name_hash,
  int              at_idx,
  int              idx_inside_at);

/**
 * Prints the automation in this Region.
 */
void
automation_region_print_automation (ZRegion * self);

int
automation_region_sort_func (const void * _a, const void * _b);

/**
 * Forces sort of the automation points.
 */
void
automation_region_force_sort (ZRegion * self);

/**
 * Adds an AutomationPoint to the Region.
 */
void
automation_region_add_ap (ZRegion * self, AutomationPoint * ap, int pub_events);

/**
 * Returns the AutomationPoint before the given
 * one.
 */
NONNULL AutomationPoint *
automation_region_get_prev_ap (ZRegion * self, AutomationPoint * ap);

/**
 * Returns the AutomationPoint after the given
 * one.
 *
 * @param check_positions Compare positions instead
 *   of just getting the next index.
 * @param check_transients Also check the transient
 *   of each object. This only matters if \ref
 *   check_positions is true.
 */
HOT AutomationPoint *
automation_region_get_next_ap (
  ZRegion *         self,
  AutomationPoint * ap,
  bool              check_positions,
  bool              check_transients);

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
  int               free);

/**
 * Returns the automation points since the last
 * recorded automation point (if the last recorded
 * automation point was before the current pos).
 */
void
automation_region_get_aps_since_last_recorded (
  ZRegion *   self,
  Position *  pos,
  GPtrArray * aps);

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
  bool       use_snapshots);

NONNULL bool
automation_region_validate (ZRegion * self);

/**
 * Frees members only but not the ZRegion itself.
 *
 * Regions should be free'd using region_free.
 */
NONNULL void
automation_region_free_members (ZRegion * self);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_REGION_H__
