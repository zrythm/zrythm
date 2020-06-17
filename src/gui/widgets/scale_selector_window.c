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

#include "actions/arranger_selections.h"
#include "audio/scale.h"
#include "audio/scale_object.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/scale_selector_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ScaleSelectorWindowWidget,
  scale_selector_window_widget,
  GTK_TYPE_WINDOW)

static gboolean
on_delete_event (
  GtkWidget *widget,
  GdkEvent  *event,
  ScaleSelectorWindowWidget * self)
{
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) self->scale);

  ArrangerSelections * before =
    arranger_selections_clone (
      (ArrangerSelections *) TL_SELECTIONS);
  ArrangerSelections * after =
    arranger_selections_clone (
      (ArrangerSelections *) TL_SELECTIONS);
  TimelineSelections * tl_after =
    (TimelineSelections *) after;
  tl_after->scale_objects[0]->scale =
    musical_scale_clone (self->descr);

  UndoableAction * ua =
    arranger_selections_action_new_edit (
      before, after,
      ARRANGER_SELECTIONS_ACTION_EDIT_SCALE,
      true);
  undo_manager_perform (
    UNDO_MANAGER, ua);
  arranger_selections_free_full (
    before);
  arranger_selections_free_full (
    after);

  return FALSE;
}

static void
creator_select_root_note (
  GtkFlowBox * box,
  GtkFlowBoxChild * child,
  ScaleSelectorWindowWidget * self)
{
  MusicalScale * clone;
  for (int i = 0; i < 12; i++)
    {
      if (self->creator_root_notes[i] != child)
        continue;

      clone =
        musical_scale_new (
          self->descr->type, i);
      musical_scale_free (self->descr);
      self->descr = clone;
      break;
    }
}

static void
creator_select_type (
  GtkFlowBox * box,
  GtkFlowBoxChild * child,
  ScaleSelectorWindowWidget * self)
{
  MusicalScale * clone;
  for (int i = 0; i < 4; i++)
    {
      if (self->creator_types[i] != child)
        continue;

      clone =
        musical_scale_new (
          i, self->descr->root_key);
      musical_scale_free (self->descr);
      self->descr = clone;
      break;
    }
}

static void
on_creator_root_note_selected_children_changed (
  GtkFlowBox * flowbox,
  ScaleSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox,
    (GtkFlowBoxForeachFunc) creator_select_root_note,
    self);
}

static void
on_creator_type_selected_children_changed (
  GtkFlowBox * flowbox,
  ScaleSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox,
    (GtkFlowBoxForeachFunc) creator_select_type,
    self);
}

static void
setup_creator_tab (
  ScaleSelectorWindowWidget * self)
{
  MusicalScale * descr =
    self->scale->scale;

#define SELECT_CHILD(flowbox, child) \
  gtk_flow_box_select_child ( \
    GTK_FLOW_BOX (self->flowbox), \
    GTK_FLOW_BOX_CHILD (self->child))

  /* set root note */
  SELECT_CHILD (
    creator_root_note_flowbox,
    creator_root_notes[descr->root_key]);

#define SELECT_SCALE_TYPE(uppercase,lowercase) \
  case SCALE_##uppercase: \
    SELECT_CHILD ( \
      creator_type_flowbox, \
      creator_type_##lowercase); \
    break

  /* set scale type */
  switch (descr->type)
    {
      SELECT_SCALE_TYPE (
        CHROMATIC, chromatic);
      SELECT_SCALE_TYPE (
        IONIAN, ionian);
      SELECT_SCALE_TYPE (
        AEOLIAN, aeolian);
      SELECT_SCALE_TYPE (
        HARMONIC_MINOR, harmonic_minor);
    default:
      /* TODO */
      break;
    }

#undef SELECT_SCALE_TYPE

#undef SELECT_CHILD

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->creator_root_note_flowbox),
    "selected-children-changed",
    G_CALLBACK (on_creator_root_note_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_type_flowbox),
    "selected-children-changed",
    G_CALLBACK (on_creator_type_selected_children_changed), self);
}

/**
 * Creates the popover.
 */
ScaleSelectorWindowWidget *
scale_selector_window_widget_new (
  ScaleObject * owner)
{
  ScaleSelectorWindowWidget * self =
    g_object_new (
      SCALE_SELECTOR_WINDOW_WIDGET_TYPE,
      NULL);

  self->scale = owner;

  gtk_window_set_transient_for (
    GTK_WINDOW (self),
    GTK_WINDOW (MAIN_WINDOW));

  setup_creator_tab (self);

  self->descr =
    musical_scale_clone (
      owner->scale);

  return self;
}

static void
scale_selector_window_widget_class_init (
  ScaleSelectorWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "scale_selector_window.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    ScaleSelectorWindowWidget, \
    child)

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
  BIND_CHILD (creator_type_chromatic);
  BIND_CHILD (creator_type_ionian);
  BIND_CHILD (creator_type_aeolian);
  BIND_CHILD (creator_type_harmonic_minor);

#undef BIND_CHILD
}

static void
scale_selector_window_widget_init (
  ScaleSelectorWindowWidget * self)
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

  self->creator_types[0] =
    self->creator_type_chromatic;
  self->creator_types[1] =
    self->creator_type_ionian;
  self->creator_types[2] =
    self->creator_type_aeolian;
  self->creator_types[3] =
    self->creator_type_harmonic_minor;

  /* set signals */
  g_signal_connect (
    G_OBJECT (self), "delete-event",
    G_CALLBACK (on_delete_event), self);
}

