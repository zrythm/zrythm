// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Center dock.
 */

#ifndef __GUI_WIDGETS_CENTER_DOCK_H__
#define __GUI_WIDGETS_CENTER_DOCK_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/libpanel_wrapper.h"

#define CENTER_DOCK_WIDGET_TYPE (center_dock_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CenterDockWidget,
  center_dock_widget,
  Z,
  CENTER_DOCK_WIDGET,
  GtkWidget)

typedef struct _LeftDockEdgeWidget  LeftDockEdgeWidget;
typedef struct _RightDockEdgeWidget RightDockEdgeWidget;
typedef struct _BotDockEdgeWidget   BotDockEdgeWidget;
typedef struct _MainNotebookWidget  MainNotebookWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CENTER_DOCK MAIN_WINDOW->center_dock

/**
 * Center dock.
 */
typedef struct _CenterDockWidget
{
  GtkWidget parent_instance;

  PanelDock * dock;

  MainNotebookWidget * main_notebook;

  LeftDockEdgeWidget *  left_dock_edge;
  RightDockEdgeWidget * right_dock_edge;
  BotDockEdgeWidget *   bot_dock_edge;

  /** Hack to remember paned position. */
  bool first_draw;
} CenterDockWidget;

void
center_dock_widget_setup (CenterDockWidget * self);

/**
 * Prepare for finalization.
 */
void
center_dock_widget_tear_down (CenterDockWidget * self);

/**
 * @}
 */

#endif
