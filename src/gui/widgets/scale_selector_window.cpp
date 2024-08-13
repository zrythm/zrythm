// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "dsp/scale.h"
#include "dsp/scale_object.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/scale_selector_window.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  ScaleSelectorWindowWidget,
  scale_selector_window_widget,
  GTK_TYPE_DIALOG)

static gboolean
on_close_request (GtkWindow * window, ScaleSelectorWindowWidget * self)
{
  self->scale->select (true, false, false);

  auto before = TL_SELECTIONS->clone_unique ();
  auto after = TL_SELECTIONS->clone_unique ();

  auto scale = dynamic_cast<ScaleObject *> (after->objects_[0].get ());
  scale->scale_ = **self->descr;

  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::EditAction> (
          *before, after.get (), ArrangerSelectionsAction::EditType::Scale,
          false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to edit scale"));
    }

  return false;
}

static void
creator_select_root_note (
  GtkFlowBox *                box,
  GtkFlowBoxChild *           child,
  ScaleSelectorWindowWidget * self)
{
  for (int i = 0; i < 12; i++)
    {
      if (self->creator_root_notes[i] != child)
        continue;

      *self->descr = std::make_unique<MusicalScale> (
        (*self->descr)->type_, ENUM_INT_TO_VALUE (MusicalNote, i));
      break;
    }
}

static void
creator_select_type (
  GtkFlowBox *                box,
  GtkFlowBoxChild *           child,
  ScaleSelectorWindowWidget * self)
{
  for (
    unsigned int i = ENUM_VALUE_TO_INT (MusicalScale::Type::Chromatic);
    i <= ENUM_VALUE_TO_INT (MusicalScale::Type::Japanese2); i++)
    {
      if (self->creator_types[i] != child)
        continue;

      *self->descr = std::make_unique<MusicalScale> (
        ENUM_INT_TO_VALUE (MusicalScale::Type, i), (*self->descr)->root_key_);
      break;
    }
}

static void
on_creator_root_note_selected_children_changed (
  GtkFlowBox *                flowbox,
  ScaleSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox, (GtkFlowBoxForeachFunc) creator_select_root_note, self);
}

static void
on_creator_type_selected_children_changed (
  GtkFlowBox *                flowbox,
  ScaleSelectorWindowWidget * self)
{
  GList *      selected_children = gtk_flow_box_get_selected_children (flowbox);
  unsigned int list_len = g_list_length (selected_children);
  z_debug ("list len %u", list_len);
  if (flowbox == self->creator_type_flowbox && list_len > 0)
    {
      gtk_flow_box_unselect_all (self->creator_type_other_flowbox);
    }
  else if (flowbox == self->creator_type_other_flowbox && list_len > 0)
    {
      gtk_flow_box_unselect_all (self->creator_type_flowbox);
    }
  g_list_free (selected_children);

  gtk_flow_box_selected_foreach (
    flowbox, (GtkFlowBoxForeachFunc) creator_select_type, self);
}

static void
setup_creator_tab (ScaleSelectorWindowWidget * self)
{
  const auto &descr = self->scale->scale_;

#define SELECT_CHILD(flowbox, child) \
  gtk_flow_box_select_child ( \
    GTK_FLOW_BOX (self->flowbox), GTK_FLOW_BOX_CHILD (self->child))

  /* set root note */
  gtk_flow_box_select_child (
    GTK_FLOW_BOX (self->creator_root_note_flowbox),
    GTK_FLOW_BOX_CHILD (
      self->creator_root_notes[ENUM_VALUE_TO_INT (descr.root_key_)]));

  /* select scale */
  gtk_flow_box_select_child (
    GTK_FLOW_BOX (self->creator_type_flowbox),
    GTK_FLOW_BOX_CHILD (self->creator_types[ENUM_VALUE_TO_INT (descr.type_)]));

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->creator_root_note_flowbox), "selected-children-changed",
    G_CALLBACK (on_creator_root_note_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_type_flowbox), "selected-children-changed",
    G_CALLBACK (on_creator_type_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_type_other_flowbox), "selected-children-changed",
    G_CALLBACK (on_creator_type_selected_children_changed), self);
}

/**
 * Creates the popover.
 */
ScaleSelectorWindowWidget *
scale_selector_window_widget_new (ScaleObject * owner)
{
  ScaleSelectorWindowWidget * self = Z_SCALE_SELECTOR_WINDOW_WIDGET (
    g_object_new (SCALE_SELECTOR_WINDOW_WIDGET_TYPE, nullptr));

  self->scale = owner;

  gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (MAIN_WINDOW));

  setup_creator_tab (self);

  *self->descr = std::make_unique<MusicalScale> (owner->scale_);

  return self;
}

static void
scale_selector_window_widget_finalize (GObject * data)
{
  ScaleSelectorWindowWidget * self = Z_SCALE_SELECTOR_WINDOW_WIDGET (data);
  delete self->descr;

  G_OBJECT_CLASS (scale_selector_window_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
scale_selector_window_widget_class_init (ScaleSelectorWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "scale_selector_window.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child (klass, ScaleSelectorWindowWidget, child)

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
  BIND_CHILD (creator_type_other_flowbox);

#undef BIND_CHILD

  GObjectClass * object_class = G_OBJECT_CLASS (_klass);
  object_class->finalize = scale_selector_window_widget_finalize;
}

static void
scale_selector_window_widget_init (ScaleSelectorWindowWidget * self)
{
  self->descr = new std::unique_ptr<MusicalScale> ();

  gtk_widget_init_template (GTK_WIDGET (self));

  self->creator_root_notes[0] = self->creator_root_note_c;
  self->creator_root_notes[1] = self->creator_root_note_cs;
  self->creator_root_notes[2] = self->creator_root_note_d;
  self->creator_root_notes[3] = self->creator_root_note_ds;
  self->creator_root_notes[4] = self->creator_root_note_e;
  self->creator_root_notes[5] = self->creator_root_note_f;
  self->creator_root_notes[6] = self->creator_root_note_fs;
  self->creator_root_notes[7] = self->creator_root_note_g;
  self->creator_root_notes[8] = self->creator_root_note_gs;
  self->creator_root_notes[9] = self->creator_root_note_a;
  self->creator_root_notes[10] = self->creator_root_note_as;
  self->creator_root_notes[11] = self->creator_root_note_b;

  gtk_flow_box_set_min_children_per_line (self->creator_root_note_flowbox, 12);

  for (
    unsigned int i = ENUM_VALUE_TO_INT (MusicalScale::Type::Chromatic);
    i <= ENUM_VALUE_TO_INT (MusicalScale::Type::Japanese2); i++)
    {
      GtkFlowBoxChild * fb_child =
        GTK_FLOW_BOX_CHILD (gtk_flow_box_child_new ());
      const auto str = MusicalScale::type_to_string (
        ENUM_INT_TO_VALUE (MusicalScale::Type, i));
      GtkWidget * lbl = gtk_label_new (str);
      gtk_flow_box_child_set_child (fb_child, lbl);
      gtk_widget_set_focusable (GTK_WIDGET (fb_child), true);
      if (i <= ENUM_VALUE_TO_INT (MusicalScale::Type::OctatonicWholeHalf))
        {
          gtk_flow_box_append (
            self->creator_type_flowbox, GTK_WIDGET (fb_child));
        }
      else
        {
          gtk_flow_box_append (
            self->creator_type_other_flowbox, GTK_WIDGET (fb_child));
        }
      self->creator_types[i] = fb_child;
    }

  gtk_flow_box_set_selection_mode (
    self->creator_type_flowbox, GTK_SELECTION_SINGLE);
  gtk_flow_box_set_selection_mode (
    self->creator_type_other_flowbox, GTK_SELECTION_SINGLE);
  gtk_flow_box_set_min_children_per_line (self->creator_type_flowbox, 4);
  gtk_flow_box_set_min_children_per_line (self->creator_type_other_flowbox, 4);

  /* set signals */
  g_signal_connect (
    G_OBJECT (self), "close-request", G_CALLBACK (on_close_request), self);
}
