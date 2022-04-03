/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Timeline panel.
 */

#ifndef __GUI_WIDGETS_MAIN_NOTEBOOK_H__
#define __GUI_WIDGETS_MAIN_NOTEBOOK_H__

#include "gui/widgets/foldable_notebook.h"

#include <gtk/gtk.h>

#define MAIN_NOTEBOOK_WIDGET_TYPE \
  (main_notebook_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MainNotebookWidget,
  main_notebook_widget,
  Z,
  MAIN_NOTEBOOK_WIDGET,
  GtkBox)

typedef struct _TimelinePanelWidget
  TimelinePanelWidget;
typedef struct _EventViewerWidget EventViewerWidget;
typedef struct _CcBindingsWidget  CcBindingsWidget;
typedef struct _PortConnectionsWidget
  PortConnectionsWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MAIN_NOTEBOOK \
  MW_CENTER_DOCK->main_notebook

typedef struct _MainNotebookWidget
{
  GtkBox parent_instance;

  FoldableNotebookWidget * foldable_notebook;

  /** Event viewr + timeline panel. */
  GtkPaned * timeline_plus_event_viewer_paned;
  TimelinePanelWidget * timeline_panel;
  EventViewerWidget *   event_viewer;
  GtkStack *            end_stack;

  GtkBox *           cc_bindings_box;
  CcBindingsWidget * cc_bindings;

  GtkBox *                port_connections_box;
  PortConnectionsWidget * port_connections;

  GtkBox * scenes_box;
} MainNotebookWidget;

void
main_notebook_widget_setup (
  MainNotebookWidget * self);

void
main_notebook_widget_refresh (
  MainNotebookWidget * self);

/**
 * Prepare for finalization.
 */
void
main_notebook_widget_tear_down (
  MainNotebookWidget * self);

/**
 * @}
 */

#endif
