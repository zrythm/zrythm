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

#include "audio/chord_descriptor.h"
#include "audio/chord_object.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_selector_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChordSelectorWindowWidget,
  chord_selector_window_widget,
  GTK_TYPE_WINDOW)

/**
 * Creates the popover.
 */
ChordSelectorWindowWidget *
chord_selector_window_widget_new (
  ChordObjectWidget * owner)
{
  ChordSelectorWindowWidget * self =
    g_object_new (
      CHORD_SELECTOR_WINDOW_WIDGET_TYPE,
      NULL);

  self->chord =
    chord_object_get_main_chord_object (
      owner->chord)->widget;

  gtk_window_set_transient_for (
    GTK_WINDOW (self),
    GTK_WINDOW (MAIN_WINDOW));

  return self;
}

static void
chord_selector_window_widget_class_init (
  ChordSelectorWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "chord_selector_window.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    ChordSelectorWindowWidget, \
    child)

  BIND_CHILD (diatonic_i);
  BIND_CHILD (diatonic_ii);
  BIND_CHILD (diatonic_iii);
  BIND_CHILD (diatonic_iv);
  BIND_CHILD (diatonic_v);
  BIND_CHILD (diatonic_vi);
  BIND_CHILD (diatonic_vii);
  BIND_CHILD (diatonic_i_lbl);
  BIND_CHILD (diatonic_ii_lbl);
  BIND_CHILD (diatonic_iii_lbl);
  BIND_CHILD (diatonic_iv_lbl);
  BIND_CHILD (diatonic_v_lbl);
  BIND_CHILD (diatonic_vi_lbl);
  BIND_CHILD (diatonic_vii_lbl);
  BIND_CHILD (creator_root_note_c);
  BIND_CHILD (creator_root_note_cs);
  BIND_CHILD (creator_root_note_d);
  BIND_CHILD (creator_root_note_ds);
  BIND_CHILD (creator_root_note_e);
  BIND_CHILD (creator_root_note_f);
  BIND_CHILD (creator_root_note_fs);
  BIND_CHILD (creator_root_note_g);
  BIND_CHILD (creator_root_note_gs);
  BIND_CHILD (creator_root_note_a);
  BIND_CHILD (creator_root_note_as);
  BIND_CHILD (creator_root_note_b);
  BIND_CHILD (creator_type_maj);
  BIND_CHILD (creator_type_min);
  BIND_CHILD (creator_type_dim);
  BIND_CHILD (creator_type_sus4);
  BIND_CHILD (creator_type_sus2);
  BIND_CHILD (creator_type_aug);
  BIND_CHILD (creator_accent_7);
  BIND_CHILD (creator_accent_j7);
  BIND_CHILD (creator_accent_b9);
  BIND_CHILD (creator_accent_9);
  BIND_CHILD (creator_accent_s9);
  BIND_CHILD (creator_accent_11);
  BIND_CHILD (creator_accent_b5_s11);
  BIND_CHILD (creator_accent_s5_b13);
  BIND_CHILD (creator_accent_6_13);
  BIND_CHILD (creator_bass_note_c);
  BIND_CHILD (creator_bass_note_cs);
  BIND_CHILD (creator_bass_note_d);
  BIND_CHILD (creator_bass_note_ds);
  BIND_CHILD (creator_bass_note_e);
  BIND_CHILD (creator_bass_note_f);
  BIND_CHILD (creator_bass_note_fs);
  BIND_CHILD (creator_bass_note_g);
  BIND_CHILD (creator_bass_note_gs);
  BIND_CHILD (creator_bass_note_a);
  BIND_CHILD (creator_bass_note_as);
  BIND_CHILD (creator_bass_note_b);
  BIND_CHILD (creator_visibility_all);
  BIND_CHILD (creator_visibility_in_scale);

#undef BIND_CHILD
}

static void
chord_selector_window_widget_init (
  ChordSelectorWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

}
