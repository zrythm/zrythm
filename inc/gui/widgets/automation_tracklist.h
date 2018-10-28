/*
 * gui/widgets/automation_tracklist.h - Automation tracklist
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_AUTOMATION_TRACKLIST_H__
#define __GUI_WIDGETS_AUTOMATION_TRACKLIST_H__

#include <gtk/gtk.h>

#define AUTOMATION_TRACKLIST_WIDGET_TYPE                  (automation_tracklist_widget_get_type ())
#define AUTOMATION_TRACKLIST_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AUTOMATION_TRACKLIST_WIDGET_TYPE, AutomationTracklistWidget))
#define AUTOMATION_TRACKLIST_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), AUTOMATION_TRACKLIST_WIDGET, AutomationTracklistWidgetClass))
#define IS_AUTOMATION_TRACKLIST_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AUTOMATION_TRACKLIST_WIDGET_TYPE))
#define IS_AUTOMATION_TRACKLIST_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), AUTOMATION_TRACKLIST_WIDGET_TYPE))
#define AUTOMATION_TRACKLIST_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), AUTOMATION_TRACKLIST_WIDGET_TYPE, AutomationTracklistWidgetClass))

typedef struct AutomationTrackWidget AutomationTrackWidget;
typedef struct TrackWidget TrackWidget;

typedef struct AutomationTracklistWidget
{
  GtkBox                  parent_instance;
  AutomationTrackWidget * automation_track_widgets[400]; ///< the automation track will
                                          ///< always be at the top part of the
                                          ///< corresponding paned
                                          ///< these stay when they are hidden
  int                     num_automation_track_widgets; ///< counter
  TrackWidget *           track_widget; ///< parent track widget
  int                     has_g_object_ref; ///< flag
} AutomationTracklistWidget;

typedef struct AutomationTracklistWidgetClass
{
  GtkBoxClass             parent_class;
} AutomationTracklistWidgetClass;

/**
 * Creates a new tracks widget and sets it to main window.
 */
AutomationTracklistWidget *
automation_tracklist_widget_new (TrackWidget * track_widget);

/**
 * Adds an automation track to the automation tracklist widget at pos.
 */
void
automation_tracklist_widget_add_automation_track (AutomationTracklistWidget * self,
                                                  AutomationTrack *           at,
                                                  int                         pos);

int
automation_tracklist_widget_get_automation_track_widget_index (
  AutomationTracklistWidget * self,
  AutomationTrackWidget * at);

/**
 * Show or hide all automation track widgets.
 */
void
automation_tracklist_widget_show (AutomationTracklistWidget * self);

#endif

