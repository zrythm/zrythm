// SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/actions/midi_mapping_action.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/dialogs/bind_cc_dialog.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/engine.h"
#include "common/utils/midi.h"
#include "common/utils/resources.h"
#include "common/utils/ui.h"

G_DEFINE_TYPE (BindCcDialogWidget, bind_cc_dialog_widget, GTK_TYPE_DIALOG)

static void
on_ok_clicked (GtkButton * btn, BindCcDialogWidget * self)
{
  if (self->perform_action)
    {
      if (self->cc[0])
        {
          try
            {
              std::array<midi_byte_t, 3> arr;
              arr[0] = self->cc[0];
              arr[1] = self->cc[1];
              arr[2] = self->cc[2];
              UNDO_MANAGER->perform (std::make_unique<MidiMappingAction> (
                arr, nullptr, *self->port));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to bind mapping"));
            }
        }
    }

  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);
}

static void
on_cancel_clicked (GtkButton * btn, BindCcDialogWidget * self)
{
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_CANCEL);
}

static gboolean
tick_cb (
  GtkWidget *          widget,
  GdkFrameClock *      frame_clock,
  BindCcDialogWidget * self)
{
  if (AUDIO_ENGINE->last_cc_captured_[0] != 0)
    {
      std::copy_n (AUDIO_ENGINE->last_cc_captured_.data (), 3, self->cc);
      char ctrl_change[60];
      int  ctrl_change_ch = midi_ctrl_change_get_ch_and_description (
        AUDIO_ENGINE->last_cc_captured_.data (), ctrl_change);

      bool port_is_toggle =
        self->port
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, self->port->id_.flags_,
          PortIdentifier::Flags::Toggle);

      gtk_widget_set_sensitive (GTK_WIDGET (self->ok_btn), true);

      if (ctrl_change_ch > 0)
        {
          auto str =
            fmt::format ("<b>Ch{} - {}</b>", ctrl_change_ch, ctrl_change);
          gtk_label_set_markup (self->lbl, str.c_str ());
        }
      else if (port_is_toggle)
        {
          char str[100];
          sprintf (
            str, "<b>%02X %02X %02X</b>", self->cc[0], self->cc[1], self->cc[2]);
          gtk_label_set_markup (self->lbl, str);
        }
      else
        {
          auto str = fmt::format (
            "<b><span foreground='red'>{}</span></b>",
            _ ("Not a control change event"));
          gtk_label_set_markup (self->lbl, str.c_str ());

          gtk_widget_set_sensitive (GTK_WIDGET (self->ok_btn), false);
        }
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Creates a new bind_cc dialog.
 */
BindCcDialogWidget *
bind_cc_dialog_widget_new (Port * port, bool perform_action)
{
  auto * self = static_cast<BindCcDialogWidget *> (
    g_object_new (BIND_CC_DIALOG_WIDGET_TYPE, nullptr));

  self->port = port;
  self->perform_action = perform_action;

  auto window = zrythm_app->get_active_window ();
  gtk_window_set_modal (GTK_WINDOW (self), true);
  gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (window->gobj ()));

  return self;
}

static void
finalize (BindCcDialogWidget * self)
{
  AUDIO_ENGINE->capture_cc_.store (false);

  G_OBJECT_CLASS (bind_cc_dialog_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
bind_cc_dialog_widget_class_init (BindCcDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "bind_cc_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, BindCcDialogWidget, x)

  BIND_CHILD (ok_btn);
  BIND_CHILD (cancel_btn);
  BIND_CHILD (lbl);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
bind_cc_dialog_widget_init (BindCcDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  AUDIO_ENGINE->capture_cc_.store (true);
  AUDIO_ENGINE->last_cc_captured_.fill (0);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb, self, nullptr);

  g_signal_connect (
    G_OBJECT (self->ok_btn), "clicked", G_CALLBACK (on_ok_clicked), self);
  g_signal_connect (
    G_OBJECT (self->cancel_btn), "clicked", G_CALLBACK (on_cancel_clicked),
    self);
}
