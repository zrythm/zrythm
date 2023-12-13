/*
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_INSPECTOR_CHORD_H__
#define __GUI_WIDGETS_INSPECTOR_CHORD_H__

#include <gtk/gtk.h>

#define INSPECTOR_CHORD_WIDGET_TYPE (inspector_chord_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorChordWidget,
  inspector_chord_widget,
  Z,
  INSPECTOR_CHORD_WIDGET,
  GtkGrid)

typedef struct ChordObject ChordObject;

typedef struct _InspectorChordWidget
{
  GtkGrid    parent_instance;
  GtkLabel * header;
  // GtkBox *            position_box;
  // GtkBox *            length_box;
  // GtkColorButton *    color;
  // GtkToggleButton *   mute_toggle;
} InspectorChordWidget;

void
inspector_chord_widget_show_chords (
  InspectorChordWidget * self,
  ChordObject **         chords,
  int                    num_chords);

#endif
