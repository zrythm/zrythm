/*
 * gui/widgets/inspector_midi.c - A inspector_midi widget
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

#include "gui/widgets/inspector_midi.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  InspectorMidiWidget,
  inspector_midi_widget,
  GTK_TYPE_GRID)

void
inspector_midi_widget_show_midi (
  InspectorMidiWidget * self,
  MidiNote **           midis,
  int                   num_midis)
{
  if (num_midis == 1)
    {
      gtk_label_set_text (self->header, "Midi");
    }
  else
    {
      char * string =
        g_strdup_printf ("Midis (%d)", num_midis);
      gtk_label_set_text (self->header, string);
      g_free (string);

      for (int i = 0; i < num_midis; i++) { }
    }
}

static void
inspector_midi_widget_class_init (
  InspectorMidiWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/org/zrythm/ui/inspector_midi.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorMidiWidget,
    position_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorMidiWidget,
    length_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorMidiWidget,
    color);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorMidiWidget,
    mute_toggle);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorMidiWidget,
    header);
}

static void
inspector_midi_widget_init (
  InspectorMidiWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
