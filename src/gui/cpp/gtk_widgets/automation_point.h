// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Automation Point.
 */

#ifndef __GUI_WIDGETS_AUTOMATION_POINT_H__
#define __GUI_WIDGETS_AUTOMATION_POINT_H__

#include "dsp/automation_point.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

constexpr int AP_WIDGET_POINT_SIZE = 6;

/**
 * Returns if the automation point (circle) is hit.
 *
 * This function assumes that the point is already
 * inside the full rect of the automation point.
 *
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 *
 * @note the transient is also checked.
 */
bool
automation_point_is_point_hit (const AutomationPoint &self, double x, double y);

/**
 * Returns if the automation curve is hit.
 *
 * This function assumes that the point is already
 * inside the full rect of the automation point.
 *
 * @param x X in global coordinates.
 * @param y Y in global coordinates.
 * @param delta_from_curve Allowed distance from the
 *   curve.
 *
 * @note the transient is also checked.
 */
bool
automation_point_is_curve_hit (
  const AutomationPoint &self,
  double                 x,
  double                 y,
  double                 delta_from_curve);

/**
 * Returns whether the cached render node for @ref
 * self needs to be invalidated.
 */
bool
automation_point_settings_changed (
  const AutomationPoint * self,
  const GdkRectangle *    draw_rect,
  bool                    timeline);

/**
 * Draws the AutomationPoint in the given cairo
 * context in absolute coordinates.
 *
 * @param rect Arranger rectangle.
 * @param layout Pango layout to draw text with.
 */
void
automation_point_draw (
  AutomationPoint * ap,
  GtkSnapshot *     snapshot,
  GdkRectangle *    rect,
  PangoLayout *     layout);

/**
 * @}
 */

#endif
