// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline ruler derived from base ruler.
 */

#ifndef __GUI_WIDGETS_TIMELINE_RULER_H__
#define __GUI_WIDGETS_TIMELINE_RULER_H__

#include "gui/widgets/ruler.h"

#include "gtk_wrapper.h"

typedef struct _RulerRangeWidget  RulerRangeWidget;
typedef struct _RulerMarkerWidget RulerMarkerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_RULER MW_TIMELINE_PANEL->ruler

/**
 * Called from ruler drag begin.
 */
void
timeline_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble       start_x,
  gdouble       start_y,
  int           height);

/**
 * Called from ruler drag end.
 */
void
timeline_ruler_on_drag_end (RulerWidget * self);

/**
 * Called from ruler drag update.
 */
void
timeline_ruler_on_drag_update (
  RulerWidget * self,
  gdouble       offset_x,
  gdouble       offset_y);

/**
 * @}
 */

#endif
