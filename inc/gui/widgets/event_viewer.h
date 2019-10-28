/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <gtk/gtk.h>

#define EVENT_VIEWER_WIDGET_TYPE \
  (event_viewer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EventViewerWidget,
  event_viewer_widget,
  Z, EVENT_VIEWER_WIDGET,
  GtkBox)

typedef struct _ArrangerWidget ArrangerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_EVENT_VIEWER \
  MW_CENTER_DOCK->event_viewer
#define MW_EDITOR_EVENT_VIEWER \
  MW_BOT_DOCK_EDGE->event_viewer

typedef enum EventViewerType
{
  EVENT_VIEWER_TYPE_TIMELINE,
  EVENT_VIEWER_TYPE_EDITOR,
} EventViewerType;

typedef enum EventViewerEventType
{
  EVENT_VIEWER_ET_REGION,
  EVENT_VIEWER_ET_MARKER,
  EVENT_VIEWER_ET_SCALE_OBJECT,
  EVENT_VIEWER_ET_MIDI_NOTE,
  EVENT_VIEWER_ET_CHORD_OBJECT,
  EVENT_VIEWER_ET_AUTOMATION_POINT,
} EventViewerEventType;

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
event_viewer_widget_refresh_for_arranger (
  ArrangerWidget *    arranger);

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
