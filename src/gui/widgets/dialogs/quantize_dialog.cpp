// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "dsp/quantize_options.h"
#include "gui/widgets/bar_slider.h"
#include "gui/widgets/dialogs/quantize_dialog.h"
#include "gui/widgets/digital_meter.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (QuantizeDialogWidget, quantize_dialog_widget, GTK_TYPE_DIALOG)

static void
on_adjust_end_toggled (GtkToggleButton * toggle, QuantizeDialogWidget * self)
{
  self->opts->adj_end_ = gtk_toggle_button_get_active (toggle);
}

static void
on_adjust_start_toggled (GtkToggleButton * toggle, QuantizeDialogWidget * self)
{
  self->opts->adj_start_ = gtk_toggle_button_get_active (toggle);
}

static void
on_cancel_clicked (GtkButton * btn, QuantizeDialogWidget * self)
{
  gtk_window_close (GTK_WINDOW (self));
}

static void
on_quantize_clicked (GtkButton * btn, QuantizeDialogWidget * self)
{
  try
    {
      if (QUANTIZE_OPTIONS_IS_EDITOR (self->opts))
        {
          ArrangerSelections * sel = CLIP_EDITOR->get_arranger_selections ();
          g_return_if_fail (sel);

          UNDO_MANAGER->perform (
            std::make_unique<ArrangerSelectionsAction::QuantizeAction> (
              *sel, *self->opts));
        }
      else if (QUANTIZE_OPTIONS_IS_TIMELINE (self->opts))
        {
          UNDO_MANAGER->perform (
            std::make_unique<ArrangerSelectionsAction::QuantizeAction> (
              *TL_SELECTIONS, *self->opts));
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to quantize selections"));
    }
}

/**
 * Creates a new quantize dialog.
 */
QuantizeDialogWidget *
quantize_dialog_widget_new (QuantizeOptions * opts)
{
  QuantizeDialogWidget * self = static_cast<QuantizeDialogWidget *> (
    g_object_new (QUANTIZE_DIALOG_WIDGET_TYPE, nullptr));

  self->opts = opts;

  gtk_toggle_button_set_active (self->adjust_start, opts->adj_start_);
  gtk_toggle_button_set_active (self->adjust_end, opts->adj_end_);

  g_signal_connect (
    G_OBJECT (self->adjust_start), "toggled",
    G_CALLBACK (on_adjust_start_toggled), self);
  g_signal_connect (
    G_OBJECT (self->adjust_end), "toggled", G_CALLBACK (on_adjust_end_toggled),
    self);

  self->note_length = digital_meter_widget_new (
    DigitalMeterType::DIGITAL_METER_TYPE_NOTE_LENGTH, &opts->note_length_,
    &opts->note_type_, _ ("note length"));
  gtk_box_append (
    GTK_BOX (self->note_length_box), GTK_WIDGET (self->note_length));
  self->note_type = digital_meter_widget_new (
    DigitalMeterType::DIGITAL_METER_TYPE_NOTE_TYPE, &opts->note_length_,
    &opts->note_type_, _ ("note type"));
  gtk_box_append (GTK_BOX (self->note_type_box), GTK_WIDGET (self->note_type));

  int w = 100, h = -1;
  self->amount = bar_slider_widget_new (
    QuantizeOptions::amount_getter, QuantizeOptions::amount_setter, opts, 0,
    100, w, h, 0, 0, UiDragMode::UI_DRAG_MODE_CURSOR, "%");
  gtk_box_append (GTK_BOX (self->amount_box), GTK_WIDGET (self->amount));
  self->swing = bar_slider_widget_new (
    QuantizeOptions::swing_getter, QuantizeOptions::swing_setter, opts, 0, 100,
    w, h, 0, 0, UiDragMode::UI_DRAG_MODE_CURSOR, "%");
  gtk_box_append (GTK_BOX (self->swing_box), GTK_WIDGET (self->swing));
  self->randomization = bar_slider_widget_new (
    QuantizeOptions::randomization_getter, QuantizeOptions::randomization_setter,
    opts, 0.f, 100.f, w, h, 0, 0, UiDragMode::UI_DRAG_MODE_CURSOR, " ticks");
  gtk_box_append (
    GTK_BOX (self->randomization_box), GTK_WIDGET (self->randomization));

  return self;
}

static void
quantize_dialog_widget_class_init (QuantizeDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "quantize_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, QuantizeDialogWidget, x)

  BIND_CHILD (cancel_btn);
  BIND_CHILD (quantize_btn);
  BIND_CHILD (adjust_start);
  BIND_CHILD (adjust_end);
  BIND_CHILD (note_length_box);
  BIND_CHILD (note_type_box);
  BIND_CHILD (amount_box);
  BIND_CHILD (swing_box);
  BIND_CHILD (randomization_box);

  gtk_widget_class_bind_template_callback (klass, on_cancel_clicked);
  gtk_widget_class_bind_template_callback (klass, on_quantize_clicked);
}

static void
quantize_dialog_widget_init (QuantizeDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
