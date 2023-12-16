// clang-format off
// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * @file
 *
 * API for selections in the AutomationArrangerWidget.
 */

#ifndef __GUI_BACKEND_AUTOMATION_SELECTIONS_H__
#define __GUI_BACKEND_AUTOMATION_SELECTIONS_H__

#include "dsp/automation_point.h"
#include "gui/backend/arranger_selections.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUTOMATION_SELECTIONS (PROJECT->automation_selections)

/**
 * Selections to be used for the AutomationArrangerWidget's
 * current selections, copying, undoing, etc.
 *
 * @extends ArrangerSelections
 */
typedef struct AutomationSelections
{
  ArrangerSelections base;

  /** Selected AutomationObject's. */
  AutomationPoint ** automation_points;
  int                num_automation_points;
  size_t             automation_points_size;

} AutomationSelections;

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
bool
automation_selections_can_be_pasted (
  AutomationSelections * ts,
  Position *             pos,
  ZRegion *              r);

/**
 * @}
 */

#endif
