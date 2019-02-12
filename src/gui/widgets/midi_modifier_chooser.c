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

#include "gui/widgets/midi_modifier_chooser.h"
#include "gui/backend/piano_roll.h"

G_DEFINE_TYPE (MidiModifierChooserWidget,
               midi_modifier_chooser_widget,
               GTK_TYPE_COMBO_BOX)

enum
{
  VALUE_COL,
  TEXT_COL
};

static GtkTreeModel *
create_icon_store (void)
{
  const int values[4] = {
    MIDI_MODIFIER_VELOCITY,
    MIDI_MODIFIER_PITCH_WHEEL,
    MIDI_MODIFIER_MOD_WHEEL,
    MIDI_MODIFIER_AFTERTOUCH,
  };
  const gchar *labels[4] = {
    "Velocity",
    "Pitch Wheel",
    "Mod Wheel",
    "Aftertouch",
  };

  GtkTreeIter iter;
  GtkListStore *store;
  gint i;

  store = gtk_list_store_new (2,
                              G_TYPE_INT,
                              G_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          VALUE_COL, values[i],
                          TEXT_COL, labels[i],
                          -1);
    }

  return GTK_TREE_MODEL (store);
}

/**
 * Updates changes in the backend to the ui
 */
void
midi_modifier_chooser_widget_setup (
  MidiModifierChooserWidget * self,
  MidiModifier *              midi_modifier)
{
  self->midi_modifier = midi_modifier;

  /* TODO select value */
}

static void
midi_modifier_chooser_widget_init (
  MidiModifierChooserWidget * self)
{
  GtkTreeModel *model;
  GtkCellRenderer *renderer;
  model = create_icon_store ();
  gtk_combo_box_set_model (GTK_COMBO_BOX (self),
                           model);
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self));
  g_object_unref (model);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self), renderer, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self), renderer,
    "text", TEXT_COL,
    NULL);
  gtk_combo_box_set_active (GTK_COMBO_BOX (self), 0);
}

static void
midi_modifier_chooser_widget_class_init (
  MidiModifierChooserWidgetClass * klass)
{
}
