// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * MIDI ruler.
 */

#ifndef __GUI_WIDGETS_EDITOR_RULER_H__
#define __GUI_WIDGETS_EDITOR_RULER_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/ruler.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define EDITOR_RULER MW_CLIP_EDITOR_INNER->ruler

/**
 * Called from ruler drag begin.
 */
void
editor_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble       start_x,
  gdouble       start_y);

void
editor_ruler_on_drag_update (
  RulerWidget * self,
  gdouble       offset_x,
  gdouble       offset_y);

void
editor_ruler_on_drag_end (RulerWidget * self);

void
editor_ruler_get_regions_in_range (
  RulerWidget *          self,
  double                 x_start,
  double                 x_end,
  std::vector<Region *> &regions);

/**
 * @}
 */

#endif
