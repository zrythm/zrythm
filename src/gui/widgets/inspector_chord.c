/*
 * gui/widgets/inspector_chord.c - A inspector_chord widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "gui/widgets/inspector_chord.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  InspectorChordWidget,
  inspector_chord_widget,
  GTK_TYPE_GRID)

void
inspector_chord_widget_show_chords (
  InspectorChordWidget * self,
  ChordObject **         chords,
  int                    num_chords)
{
  if (num_chords == 1)
    {
      gtk_label_set_text (self->header, "Chord");
    }
  else
    {
      char * string = g_strdup_printf (
        "Chords (%d)", num_chords);
      gtk_label_set_text (self->header, string);
      g_free (string);

      for (int i = 0; i < num_chords; i++) { }
    }
}

static void
inspector_chord_widget_class_init (
  InspectorChordWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "inspector_chord.ui");

  /*gtk_widget_class_bind_template_child (*/
  /*klass,*/
  /*InspectorChordWidget,*/
  /*position_box);*/
  /*gtk_widget_class_bind_template_child (*/
  /*klass,*/
  /*InspectorChordWidget,*/
  /*length_box);*/
  /*gtk_widget_class_bind_template_child (*/
  /*klass,*/
  /*InspectorChordWidget,*/
  /*color);*/
  /*gtk_widget_class_bind_template_child (*/
  /*klass,*/
  /*InspectorChordWidget,*/
  /*mute_toggle);*/
  gtk_widget_class_bind_template_child (
    klass, InspectorChordWidget, header);
}

static void
inspector_chord_widget_init (
  InspectorChordWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
