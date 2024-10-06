// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/chord_descriptor.h"
#include "common/dsp/chord_object.h"
#include "common/utils/flags.h"
#include "common/utils/resources.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/chord_editor_space.h"
#include "gui/backend/gtk_widgets/chord_key.h"
#include "gui/backend/gtk_widgets/chord_selector_window.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/piano_keyboard.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

G_DEFINE_TYPE (ChordKeyWidget, chord_key_widget, GTK_TYPE_GRID)

static ChordDescriptor *
get_chord_descr (ChordKeyWidget * self)
{
  return &CHORD_EDITOR->chords_[self->chord_idx];
}

static void
on_choose_chord_btn_clicked (GtkButton * btn, ChordKeyWidget * self)
{
  chord_selector_window_widget_present (
    self->chord_idx, GTK_WIDGET (MAIN_WINDOW));
}

static void
on_invert_btn_clicked (GtkButton * btn, ChordKeyWidget * self)
{
  const ChordDescriptor * descr = get_chord_descr (self);
  ChordDescriptor         descr_clone = *descr;

  if (btn == self->invert_prev_btn)
    {
      if (descr->get_min_inversion () != descr->inversion_)
        {
          descr_clone.inversion_--;
          CHORD_EDITOR->apply_single_chord (
            descr_clone, self->chord_idx, F_UNDOABLE);
        }
    }
  else if (btn == self->invert_next_btn)
    {
      if (descr->get_max_inversion () != descr->inversion_)
        {
          descr_clone.inversion_++;
          CHORD_EDITOR->apply_single_chord (
            descr_clone, self->chord_idx, F_UNDOABLE);
        }
    }

  /*chord_key_widget_refresh (self);*/
}

void
chord_key_widget_refresh (ChordKeyWidget * self)
{
  ChordDescriptor * descr = get_chord_descr (self);
  gtk_label_set_text (self->chord_lbl, descr->to_string ().c_str ());
  piano_keyboard_widget_refresh (self->piano);
}

/**
 * Creates a ChordKeyWidget for the given
 * MIDI note descriptor.
 */
ChordKeyWidget *
chord_key_widget_new (int idx)
{
  auto * self = static_cast<ChordKeyWidget *> (
    g_object_new (CHORD_KEY_WIDGET_TYPE, nullptr));

  self->chord_idx = idx;

  /* add piano widget */
  self->piano = piano_keyboard_widget_new_for_chord_key (idx);
  gtk_widget_set_size_request (GTK_WIDGET (self->piano), 216, 24);
  gtk_box_append (self->piano_box, GTK_WIDGET (self->piano));

  chord_key_widget_refresh (self);

  return self;
}

static void
chord_key_widget_class_init (ChordKeyWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "chord-key");

  resources_set_class_template (klass, "chord_key.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ChordKeyWidget, x)

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

  gtk_widget_set_halign (GTK_WIDGET (self->btn_box), GTK_ALIGN_END);

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
