// SPDX-FileCopyrightText: © 2018-2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_AUTOMATION_ARRANGER_H__
#define __GUI_WIDGETS_AUTOMATION_ARRANGER_H__

#include "dsp/position.h"
#include "gui/cpp/gtk_widgets/arranger.h"

#include "gtk_wrapper.h"

class AutomationPoint;
class AutomationCurve;
TYPEDEF_STRUCT_UNDERSCORED (AutomationPointWidget);
TYPEDEF_STRUCT_UNDERSCORED (AutomationCurveWidget);
class SnapGrid;
TYPEDEF_STRUCT_UNDERSCORED (ArrangerWidget);
class AutomationTrack;
TYPEDEF_STRUCT_UNDERSCORED (RegionWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_AUTOMATION_ARRANGER MW_AUTOMATION_EDITOR_SPACE->arranger

ArrangerCursor
automation_arranger_widget_get_cursor (ArrangerWidget * self, Tool tool);

/** Padding to leave before and after the usable vertical range for automation. */
constexpr int AUTOMATION_ARRANGER_VPADDING = 4;

/**
 * Create an AutomationPointat the given Position in the given Track's
 * AutomationTrack.
 *
 * @param pos The pre-snapped position.
 */
void
automation_arranger_widget_create_ap (
  ArrangerWidget *   self,
  const Position *   pos,
  const double       start_y,
  AutomationRegion * region,
  bool               autofilling);

/**
 * Change curviness of selected curves.
 */
void
automation_arranger_widget_resize_curves (ArrangerWidget * self, double offset_y);

/**
 * Generate a context menu at x, y.
 *
 * @return The given updated menu or a new menu.
 */
GMenu *
automation_arranger_widget_gen_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y);

/**
 * Called when using the edit tool.
 *
 * @return Whether an automation point was moved.
 */
bool
automation_arranger_move_hit_aps (ArrangerWidget * self, double x, double y);

void
automation_arranger_on_drag_end (ArrangerWidget * self);

/**
 * @}
 */

#endif
