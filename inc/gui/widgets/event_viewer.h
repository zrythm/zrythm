/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Event viewer.
 */

#ifndef __GUI_WIDGETS_EVENT_VIEWER_H__
#define __GUI_WIDGETS_EVENT_VIEWER_H__

#include "audio/region_identifier.h"

#include <gtk/gtk.h>

#define EVENT_VIEWER_WIDGET_TYPE \
  (event_viewer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EventViewerWidget,
  event_viewer_widget,
  Z, EVENT_VIEWER_WIDGET,
  GtkBox)

typedef struct _ArrangerWidget ArrangerWidget;
typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_EVENT_VIEWER \
  MW_MAIN_NOTEBOOK->event_viewer
#define MW_EDITOR_EVENT_VIEWER \
  MW_BOT_DOCK_EDGE->event_viewer

typedef enum EventViewerType
{
  EVENT_VIEWER_TYPE_TIMELINE,
  EVENT_VIEWER_TYPE_EDITOR,
} EventViewerType;

typedef struct _EventViewerWidget
{
  GtkBox                 parent_instance;

  /** The tree view. */
  GtkTreeModel *         model;
  GtkTreeView *          treeview;

  /** Type. */
  EventViewerType        type;

  /** Used by the editor EV to check if it should
   * readd the columns. */
  RegionType             region_type;

  /** Temporary flag. */
  bool                   marking_selected_objs;
} EventViewerWidget;

/**
 * Called to update the models.
 */
void
event_viewer_widget_refresh (
  EventViewerWidget * self);

/**
 * Convenience function.
 */
void
event_viewer_widget_refresh_for_selections (
  ArrangerSelections * sel);

/**
 * Convenience function.
 */
void
event_viewer_widget_refresh_for_arranger (
  ArrangerWidget *    arranger);

EventViewerWidget *
event_viewer_widget_new (void);

/**
 * Sets up the event viewer.
 */
void
event_viewer_widget_setup (
  EventViewerWidget * self,
  EventViewerType     type);

/**
 * @}
 */

#endif
