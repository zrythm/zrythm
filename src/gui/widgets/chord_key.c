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
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_key.h"
#include "gui/widgets/chord_selector_window.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_keyboard.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/midi.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ChordKeyWidget,
  chord_key_widget,
  GTK_TYPE_GRID)

static ChordDescriptor *
get_chord_descr (ChordKeyWidget * self)
{
  return CHORD_EDITOR->chords[self->chord_idx];
}

static void
on_choose_chord_btn_clicked (
  GtkButton *      btn,
  ChordKeyWidget * self)
{
  ChordSelectorWindowWidget * chord_selector =
    chord_selector_window_widget_new (
      self->chord_idx);

  gtk_window_present (GTK_WINDOW (chord_selector));
}

static void
on_invert_btn_clicked (
  GtkButton *      btn,
  ChordKeyWidget * self)
{
  const ChordDescriptor * descr =
    get_chord_descr (self);
  ChordDescriptor * descr_clone =
    chord_descriptor_clone (descr);

  if (btn == self->invert_prev_btn)
    {
      if (
        chord_descriptor_get_min_inversion (descr)
        != descr->inversion)
        {
          descr_clone->inversion--;
          chord_editor_apply_single_chord (
            CHORD_EDITOR, descr_clone,
            self->chord_idx, F_UNDOABLE);
        }
    }
  else if (btn == self->invert_next_btn)
    {
      if (
        chord_descriptor_get_max_inversion (descr)
        != descr->inversion)
        {
          descr_clone->inversion++;
          chord_editor_apply_single_chord (
            CHORD_EDITOR, descr_clone,
            self->chord_idx, F_UNDOABLE);
        }
    }

  /*chord_key_widget_refresh (self);*/
}

void
chord_key_widget_refresh (ChordKeyWidget * self)
{
  char              str[120];
  ChordDescriptor * descr = get_chord_descr (self);
  chord_descriptor_to_string (descr, str);
  gtk_label_set_text (self->chord_lbl, str);

  piano_keyboard_widget_refresh (self->piano);
}

/**
 * Creates a ChordKeyWidget for the given
 * MIDI note descriptor.
 */
ChordKeyWidget *
chord_key_widget_new (int idx)
{
  ChordKeyWidget * self =
    g_object_new (CHORD_KEY_WIDGET_TYPE, NULL);

  self->chord_idx = idx;

  /* add piano widget */
  self->piano =
    piano_keyboard_widget_new_for_chord_key (idx);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->piano), 216, 24);
  gtk_box_append (
    self->piano_box, GTK_WIDGET (self->piano));

  chord_key_widget_refresh (self);

  return self;
}

static void
chord_key_widget_class_init (
  ChordKeyWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "chord-key");

  resources_set_class_template (
    klass, "chord_key.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ChordKeyWidget, x)

  BIND_CHILD (chord_lbl);
  BIND_CHILD (piano_box);
  BIND_CHILD (btn_box);
  BIND_CHILD (choose_chord_btn);
  BIND_CHILD (invert_prev_btn);
  BIND_CHILD (invert_next_btn);

#undef BIND_CHILD
}

static void
chord_key_widget_init (ChordKeyWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_halign (
    GTK_WIDGET (self->btn_box), GTK_ALIGN_END);

  g_signal_connect (
    G_OBJECT (self->choose_chord_btn), "clicked",
    G_CALLBACK (on_choose_chord_btn_clicked), self);
  g_signal_connect (
    G_OBJECT (self->invert_prev_btn), "clicked",
    G_CALLBACK (on_invert_btn_clicked), self);
  g_signal_connect (
    G_OBJECT (self->invert_next_btn), "clicked",
    G_CALLBACK (on_invert_btn_clicked), self);
}
