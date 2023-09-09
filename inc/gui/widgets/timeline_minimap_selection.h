// SPDX-FileCopyrightText: © 2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline minimap selection.
 */

#ifndef __GUI_WIDGETS_TIMELINE_MINIMAP_SELECTION_H__
#define __GUI_WIDGETS_TIMELINE_MINIMAP_SELECTION_H__

#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define TIMELINE_MINIMAP_SELECTION_WIDGET_TYPE \
  (timeline_minimap_selection_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineMinimapSelectionWidget,
  timeline_minimap_selection_widget,
  Z,
  TIMELINE_MINIMAP_SELECTION_WIDGET,
  GtkWidget)

typedef struct _TimelineMinimapSelectionWidget
{
  GtkWidget parent_instance;

  UiCursorState cursor;

  /** Pointer back to parent. */
  TimelineMinimapWidget * parent;
} TimelineMinimapSelectionWidget;

TimelineMinimapSelectionWidget *
timeline_minimap_selection_widget_new (TimelineMinimapWidget * parent);

#if 0
void
timeline_minimap_selection_widget_on_motion (
  GtkWidget *      widget,
  GdkMotionEvent * event,
  gpointer         user_data);
#endif

/**
 * @}
 */

#endif
