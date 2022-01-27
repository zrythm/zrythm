/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/chord_descriptor.h"
#include "gui/widgets/chord.h"
#include "gui/widgets/chord_pad.h"
#include "utils/gtk.h"
#include "utils/resources.h"

G_DEFINE_TYPE (
  ChordPadWidget, chord_pad_widget, GTK_TYPE_GRID)

void
chord_pad_widget_setup (
  ChordPadWidget * self)
{
  for (int i = 0; i < 12; i++)
    {
      chord_widget_refresh (self->chords[i], i);
    }
}

void
chord_pad_widget_refresh (
  ChordPadWidget * self)
{
  g_debug ("refreshing chord pad...");

  chord_pad_widget_setup (self);
}

ChordPadWidget *
chord_pad_widget_new (void)
{
  ChordPadWidget * self =
    g_object_new (
      CHORD_PAD_WIDGET_TYPE, NULL);

  return self;
}

static void
chord_pad_widget_init (
  ChordPadWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_button_set_icon_name (
    self->save_preset_btn, "document-save-as");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->save_preset_btn),
    "app.save-chord-preset");

  gtk_menu_button_set_icon_name (
    self->load_preset_btn, "document-open");

  gtk_actionable_set_detailed_action_name (
    GTK_ACTIONABLE (self->transpose_up_btn),
    "app.transpose-chord-pad::up");
  gtk_actionable_set_detailed_action_name (
    GTK_ACTIONABLE (self->transpose_down_btn),
    "app.transpose-chord-pad::down");

  for (int i = 0; i < 12; i++)
    {
      ChordWidget * chord = chord_widget_new ();
      self->chords[i] = chord;
      gtk_grid_attach (
        self->chords_grid, GTK_WIDGET (chord),
        i % 6, i / 6, 1, 1);
    }
}

static void
chord_pad_widget_class_init (
  ChordPadWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "chord_pad.ui");

  gtk_widget_class_set_css_name (
    klass, "chord-pad");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ChordPadWidget, x)

  BIND_CHILD (chords_grid);
  BIND_CHILD (save_preset_btn);
  BIND_CHILD (load_preset_btn);
  BIND_CHILD (transpose_up_btn);
  BIND_CHILD (transpose_down_btn);

#undef BIND_CHILD
}
