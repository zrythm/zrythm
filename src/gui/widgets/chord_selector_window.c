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

#include "actions/edit_chord_action.h"
#include "audio/chord_descriptor.h"
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/scale_object.h"
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

static gboolean
on_delete_event (
  GtkWidget *widget,
  GdkEvent  *event,
  ChordSelectorWindowWidget * self)
{
  UndoableAction * ua =
    edit_chord_action_new (
      self->chord->chord,
      self->descr);
  undo_manager_perform (
    UNDO_MANAGER, ua);

  return FALSE;
}

static void
creator_select_root_note (
  GtkFlowBox * box,
  GtkFlowBoxChild * child,
  ChordSelectorWindowWidget * self)
{
  for (int i = 0; i < 12; i++)
    {
      if (self->creator_root_notes[i] != child)
        continue;

      self->descr->root_note = i;
    }
}

static void
creator_select_type (
  GtkFlowBox * box,
  GtkFlowBoxChild * child,
  ChordSelectorWindowWidget * self)
{
  for (int i = 0; i < NUM_CHORD_TYPES; i++)
    {
      if (self->creator_types[i] != child)
        continue;

      self->descr->type = i;
    }
}

static void
creator_select_accent (
  GtkFlowBox * box,
  GtkFlowBoxChild * child,
  ChordSelectorWindowWidget * self)
{
  for (int i = 0; i < NUM_CHORD_ACCENTS - 1; i++)
    {
      if (self->creator_accents[i] != child)
        continue;

      self->descr->accent = i + 1;
    }
}

static void
on_creator_root_note_selected_children_changed (
  GtkFlowBox * flowbox,
  ChordSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox,
    (GtkFlowBoxForeachFunc) creator_select_root_note,
    self);
}

static void
on_creator_type_selected_children_changed (
  GtkFlowBox * flowbox,
  ChordSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox,
    (GtkFlowBoxForeachFunc) creator_select_type,
    self);
}

static void
on_creator_accent_selected_children_changed (
  GtkFlowBox * flowbox,
  ChordSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox,
    (GtkFlowBoxForeachFunc) creator_select_accent,
    self);
}

static void
on_creator_bass_note_selected_children_changed (
  GtkFlowBox * flowbox,
  ChordSelectorWindowWidget * self)
{

}

static void
setup_creator_tab (
  ChordSelectorWindowWidget * self)
{
  ChordDescriptor * descr =
    self->chord->chord->descr;

#define SELECT_CHILD(flowbox, child) \
  gtk_flow_box_select_child ( \
    GTK_FLOW_BOX (self->flowbox), \
    GTK_FLOW_BOX_CHILD (self->child))

  /* set root note */
  SELECT_CHILD (
    creator_root_note_flowbox,
    creator_root_notes[descr->root_note]);

#define SELECT_CHORD_TYPE(uppercase,lowercase) \
  case CHORD_TYPE_##uppercase: \
    SELECT_CHILD ( \
      creator_type_flowbox, \
      creator_type_##lowercase); \
    break

  /* set chord type */
  switch (descr->type)
    {
      SELECT_CHORD_TYPE (MAJ, maj);
      SELECT_CHORD_TYPE (MIN, min);
      SELECT_CHORD_TYPE (DIM, dim);
      SELECT_CHORD_TYPE (SUS4, sus4);
      SELECT_CHORD_TYPE (SUS2, sus2);
      SELECT_CHORD_TYPE (AUG, aug);
    default:
      /* TODO */
      break;
    }

#undef SELECT_CHORD_TYPE

#define SELECT_CHORD_ACC(uppercase,lowercase) \
  case CHORD_ACC_##uppercase: \
    SELECT_CHILD ( \
      creator_accent_flowbox, \
      creator_accent_##lowercase); \
    break

  /* set accent */
  switch (descr->accent)
    {
      SELECT_CHORD_ACC (7, 7);
      SELECT_CHORD_ACC (j7, j7);
      SELECT_CHORD_ACC (b9, b9);
      SELECT_CHORD_ACC (9, 9);
      default:
        break;
    }

#undef SELECT_CHORD_ACC

  /* set bass note */
  if (descr->has_bass)
    {
      SELECT_CHILD (
        creator_bass_note_flowbox,
        creator_bass_notes[descr->bass_note]);
    }

#undef SELECT_CHILD

  gtk_widget_set_sensitive (
    GTK_WIDGET (self->creator_visibility_in_scale),
    self->scale != NULL);

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->creator_root_note_flowbox),
    "selected-children-changed",
    G_CALLBACK (on_creator_root_note_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_type_flowbox),
    "selected-children-changed",
    G_CALLBACK (on_creator_type_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_accent_flowbox),
    "selected-children-changed",
    G_CALLBACK (on_creator_accent_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_bass_note_flowbox),
    "selected-children-changed",
    G_CALLBACK (on_creator_bass_note_selected_children_changed), self);
}

static void
setup_diatonic_tab (
  ChordSelectorWindowWidget * self)
{
  if (self->scale)
    {
      /* TODO */
    }
  else
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->diatonic_flowbox), 0);
    }
}

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
  self->scale =
    chord_track_get_scale_at_pos (
      P_CHORD_TRACK,
      &owner->chord->pos);

  gtk_window_set_transient_for (
    GTK_WINDOW (self),
    GTK_WINDOW (MAIN_WINDOW));

  setup_creator_tab (self);
  setup_diatonic_tab (self);

  self->descr =
    chord_descriptor_clone (
      owner->chord->descr);

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

  BIND_CHILD (diatonic_flowbox);
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
  BIND_CHILD (creator_root_note_flowbox);
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
  BIND_CHILD (creator_type_flowbox);
  BIND_CHILD (creator_type_maj);
  BIND_CHILD (creator_type_min);
  BIND_CHILD (creator_type_dim);
  BIND_CHILD (creator_type_sus4);
  BIND_CHILD (creator_type_sus2);
  BIND_CHILD (creator_type_aug);
  BIND_CHILD (creator_accent_flowbox);
  BIND_CHILD (creator_accent_7);
  BIND_CHILD (creator_accent_j7);
  BIND_CHILD (creator_accent_b9);
  BIND_CHILD (creator_accent_9);
  BIND_CHILD (creator_accent_s9);
  BIND_CHILD (creator_accent_11);
  BIND_CHILD (creator_accent_b5_s11);
  BIND_CHILD (creator_accent_s5_b13);
  BIND_CHILD (creator_accent_6_13);
  BIND_CHILD (creator_bass_note_flowbox);
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

  self->creator_root_notes[0] =
    self->creator_root_note_c;
  self->creator_root_notes[1] =
    self->creator_root_note_cs;
  self->creator_root_notes[2] =
    self->creator_root_note_d;
  self->creator_root_notes[3] =
    self->creator_root_note_ds;
  self->creator_root_notes[4] =
    self->creator_root_note_e;
  self->creator_root_notes[5] =
    self->creator_root_note_f;
  self->creator_root_notes[6] =
    self->creator_root_note_fs;
  self->creator_root_notes[7] =
    self->creator_root_note_g;
  self->creator_root_notes[8] =
    self->creator_root_note_gs;
  self->creator_root_notes[9] =
    self->creator_root_note_a;
  self->creator_root_notes[10] =
    self->creator_root_note_as;
  self->creator_root_notes[11] =
    self->creator_root_note_b;

  self->creator_bass_notes[0] =
    self->creator_bass_note_c;
  self->creator_bass_notes[1] =
    self->creator_bass_note_cs;
  self->creator_bass_notes[2] =
    self->creator_bass_note_d;
  self->creator_bass_notes[3] =
    self->creator_bass_note_ds;
  self->creator_bass_notes[4] =
    self->creator_bass_note_e;
  self->creator_bass_notes[5] =
    self->creator_bass_note_f;
  self->creator_bass_notes[6] =
    self->creator_bass_note_fs;
  self->creator_bass_notes[7] =
    self->creator_bass_note_g;
  self->creator_bass_notes[8] =
    self->creator_bass_note_gs;
  self->creator_bass_notes[9] =
    self->creator_bass_note_a;
  self->creator_bass_notes[10] =
    self->creator_bass_note_as;
  self->creator_bass_notes[11] =
    self->creator_bass_note_b;

  self->creator_types[0] =
    self->creator_type_maj;
  self->creator_types[1] =
    self->creator_type_min;
  self->creator_types[2] =
    self->creator_type_dim;
  self->creator_types[3] =
    self->creator_type_sus4;
  self->creator_types[4] =
    self->creator_type_sus2;
  self->creator_types[5] =
    self->creator_type_aug;

  self->creator_accents[0] =
    self->creator_accent_7;
  self->creator_accents[1] =
    self->creator_accent_j7;
  self->creator_accents[2] =
    self->creator_accent_b9;
  self->creator_accents[3] =
    self->creator_accent_9;
  self->creator_accents[4] =
    self->creator_accent_s9;
  self->creator_accents[5] =
    self->creator_accent_11;
  self->creator_accents[6] =
    self->creator_accent_b5_s11;
  self->creator_accents[7] =
    self->creator_accent_s5_b13;
  self->creator_accents[8] =
    self->creator_accent_6_13;

  /* set signals */
  g_signal_connect (
    G_OBJECT (self), "delete-event",
    G_CALLBACK (on_delete_event), self);
}
