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

#ifndef __GUI_WIDGETS_INSPECTOR_EDITOR_H__
#define __GUI_WIDGETS_INSPECTOR_EDITOR_H__

#include <gtk/gtk.h>

#define INSPECTOR_EDITOR_WIDGET_TYPE \
  (inspector_editor_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorEditorWidget,
  inspector_editor_widget,
  Z,
  INSPECTOR_EDITOR_WIDGET,
  GtkBox)

typedef struct _InspectorEditorWidget
{
  GtkBox     parent_instance;
  GtkLabel * header;
} InspectorEditorWidget;

/**
 * Shows the appropriate information based on if
 * the Audio or MIDI editor are visible.
 */
void
inspector_editor_widget_show (
  InspectorEditorWidget * self);

#endif
