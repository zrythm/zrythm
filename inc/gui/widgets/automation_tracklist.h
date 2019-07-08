/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * A multi-paned holding multiple
 * AutomationLaneWidget s.
 */

#ifndef __GUI_WIDGETS_AUTOMATION_TRACKLIST_H__
#define __GUI_WIDGETS_AUTOMATION_TRACKLIST_H__

#include "gui/widgets/dzl/dzl-multi-paned.h"

#include <gtk/gtk.h>

#define AUTOMATION_TRACKLIST_WIDGET_TYPE                  (automation_tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomationTracklistWidget,
  automation_tracklist_widget,
  Z, AUTOMATION_TRACKLIST_WIDGET,
  DzlMultiPaned)

typedef struct _AutomationTrackWidget
  AutomationTrackWidget;
typedef struct _TrackWidget TrackWidget;
typedef struct AutomationTracklist AutomationTracklist;

/**
 * A DzlMultiPaned holding multiple
 * AutomationLaneWidgets.
 */
typedef struct _AutomationTracklistWidget
{
  DzlMultiPaned           parent_instance;

  /**
   * The backend.
   */
  AutomationTracklist *   automation_tracklist;
} AutomationTracklistWidget;

/**
 * Creates a new AutomationTracklistWidget.
 */
AutomationTracklistWidget *
automation_tracklist_widget_new (
  AutomationTracklist * automation_tracklist);

/**
 * Show or hide all automation track widgets.
 */
void
automation_tracklist_widget_refresh (
  AutomationTracklistWidget * self);

#endif
