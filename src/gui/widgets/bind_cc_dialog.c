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

/**
 * \file
 *
 * BindCc dialog.
 */

#include "audio/engine.h"
#include "audio/midi.h"
#include "gui/widgets/bind_cc_dialog.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  BindCcDialogWidget,
  bind_cc_dialog_widget,
  GTK_TYPE_DIALOG)

static void
on_ok_clicked (
  GtkButton * btn,
  BindCcDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);
}

static void
on_cancel_clicked (
  GtkButton * btn,
  BindCcDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self), GTK_RESPONSE_CANCEL);
}

static gboolean
tick_cb (
  GtkWidget *          widget,
  GdkFrameClock *      frame_clock,
  BindCcDialogWidget * self)
{
  if (AUDIO_ENGINE->last_cc[0] != 0)
    {
      memcpy (
        self->cc, AUDIO_ENGINE->last_cc,
        sizeof (midi_byte_t) * 3);
      char ctrl_change[60];
      int ch =
        midi_ctrl_change_get_ch_and_description (
          AUDIO_ENGINE->last_cc, ctrl_change);

      gtk_widget_set_sensitive (
        GTK_WIDGET (self->ok_btn), ch > 0);

      if (ch > 0)
        {
          char str[100];
          sprintf (
            str, "<b>Ch%d - %s</b>",
            ch, ctrl_change);
          gtk_label_set_markup (
            self->lbl, str);
        }
      else
        {
          char str[100];
          sprintf (
            str,
            "<b><span foreground='red'>%s</span></b>",
            _("Not a control change event"));
          gtk_label_set_markup (
            self->lbl, str);
        }
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Creates a new bind_cc dialog.
 */
BindCcDialogWidget *
bind_cc_dialog_widget_new ()
{
  BindCcDialogWidget * self =
    g_object_new (BIND_CC_DIALOG_WIDGET_TYPE, NULL);

  return self;
}

static void
finalize (
  BindCcDialogWidget * self)
{
  AUDIO_ENGINE->capture_cc = 0;

  G_OBJECT_CLASS (
    bind_cc_dialog_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
bind_cc_dialog_widget_class_init (
  BindCcDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "bind_cc_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, BindCcDialogWidget, x)

  BIND_CHILD (ok_btn);
  BIND_CHILD (cancel_btn);
  BIND_CHILD (lbl);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
bind_cc_dialog_widget_init (
  BindCcDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  AUDIO_ENGINE->capture_cc = 1;
  AUDIO_ENGINE->last_cc[0] = 0;
  AUDIO_ENGINE->last_cc[1] = 0;
  AUDIO_ENGINE->last_cc[2] = 0;

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    (GtkTickCallback) tick_cb,
    self, NULL);

  g_signal_connect (
    G_OBJECT (self->ok_btn), "clicked",
    G_CALLBACK (on_ok_clicked), self);
  g_signal_connect (
    G_OBJECT (self->cancel_btn), "clicked",
    G_CALLBACK (on_cancel_clicked), self);
}
