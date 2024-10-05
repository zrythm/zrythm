// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline panel.
 */

#ifndef __GUI_WIDGETS_MAIN_NOTEBOOK_H__
#define __GUI_WIDGETS_MAIN_NOTEBOOK_H__

#include "gui/cpp/gtk_widgets/foldable_notebook.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/utils/types.h"

#define MAIN_NOTEBOOK_WIDGET_TYPE (main_notebook_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MainNotebookWidget,
  main_notebook_widget,
  Z,
  MAIN_NOTEBOOK_WIDGET,
  GtkWidget)

TYPEDEF_STRUCT_UNDERSCORED (TimelinePanelWidget);
TYPEDEF_STRUCT_UNDERSCORED (EventViewerWidget);
TYPEDEF_STRUCT_UNDERSCORED (CcBindingsWidget);
TYPEDEF_STRUCT_UNDERSCORED (PortConnectionsWidget);
TYPEDEF_STRUCT_UNDERSCORED (PanelFrame);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MAIN_NOTEBOOK MW_CENTER_DOCK->main_notebook

typedef struct _MainNotebookWidget
{
  GtkWidget parent_instance;

  PanelFrame * panel_frame;

  /** Event viewr + timeline panel. */
  GtkPaned *            timeline_plus_event_viewer_paned;
  TimelinePanelWidget * timeline_panel;
  EventViewerWidget *   event_viewer;

  GtkBox *           cc_bindings_box;
  CcBindingsWidget * cc_bindings;

  GtkBox *                port_connections_box;
  PortConnectionsWidget * port_connections;

  GtkBox * scenes_box;
} MainNotebookWidget;

void
main_notebook_widget_setup (MainNotebookWidget * self);

void
main_notebook_widget_refresh (MainNotebookWidget * self);

/**
 * Prepare for finalization.
 */
void
main_notebook_widget_tear_down (MainNotebookWidget * self);

/**
 * @}
 */

#endif
