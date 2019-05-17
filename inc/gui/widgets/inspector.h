/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_INSPECTOR_H__
#define __GUI_WIDGETS_INSPECTOR_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Widgets
 *
 * @{
 */

#define INSPECTOR_WIDGET_TYPE \
  (inspector_widget_get_type ())
G_DECLARE_FINAL_TYPE (InspectorWidget,
                      inspector_widget,
                      Z,
                      INSPECTOR_WIDGET,
                      GtkStack)

#define MW_INSPECTOR MW_LEFT_DOCK_EDGE->inspector

typedef struct _InspectorTrackWidget
  InspectorTrackWidget;
typedef struct _InspectorEditorWidget
  InspectorEditorWidget;
typedef struct _InspectorPluginWidget
  InspectorPluginWidget;

typedef struct _InspectorWidget
{
  GtkStack                  parent_instance;

  /** For TracklistSelections. */
  InspectorTrackWidget *    track;

  /** For editor (midi/audio/etc.). */
  InspectorEditorWidget *   editor;

  /** For MixerSelections. */
  InspectorPluginWidget *   plugin;

} InspectorWidget;

/**
 * Creates the inspector widget.
 *
 * Only once per project.
 */
InspectorWidget *
inspector_widget_new ();

/**
 * Refreshes the inspector widget (shows current
 * selections.
 *
 * Uses Project->last_selection to decide which
 * stack to show.
 */
void
inspector_widget_refresh (
  InspectorWidget * self);

/**
 * @}
 */

#endif
