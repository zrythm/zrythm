/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Generic dialog.
 */

#include <stdio.h>

#include "audio/engine.h"
#include "gui/widgets/dialogs/generic_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  GenericProgressDialogWidget,
  generic_progress_dialog_widget,
  GTK_TYPE_DIALOG)

#define GET_PRIVATE(x) \
  GenericProgressDialogWidgetPrivate * prv = \
     generic_progress_dialog_widget_get_instance_private (x)

static void
on_closed (
  GtkDialog *                  dialog,
  GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  prv->progress_info->cancelled = true;
}

static void
on_ok_clicked (
  GtkButton * btn,
  GenericProgressDialogWidget * self)
{
  gtk_dialog_response (GTK_DIALOG (self), 0);
}

static void
on_cancel_clicked (
  GtkButton * btn,
  GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  prv->progress_info->cancelled = true;
}

static gboolean
tick_cb (
  GtkWidget *                   widget,
  GdkFrameClock *               frame_clock,
  GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  GenericProgressInfo * info = prv->progress_info;

  gtk_progress_bar_set_fraction (
    GTK_PROGRESS_BAR (widget), info->progress);

  if (math_doubles_equal (info->progress, 1.0) ||
      info->has_error || info->cancelled)
    {
      if (prv->autoclose || info->cancelled)
        {
          gtk_dialog_response (
            GTK_DIALOG (self), 0);
        }
      else
        {
          gtk_widget_set_visible (
            GTK_WIDGET (prv->ok), true);
          gtk_widget_set_visible (
            GTK_WIDGET (prv->cancel), false);
          for (size_t i = 0;
               i < prv->num_extra_buttons; i++)
            {
              GenericProgressDialogButton * btn =
                &prv->extra_buttons[i];
              if (btn->only_on_finish)
                {
                  gtk_widget_set_visible (
                    GTK_WIDGET (btn->btn), true);
                }
            }
          gtk_label_set_text (
            prv->label, info->label_done_str);
        }

      if (info->has_error)
        {
          GtkWindow * transient_parent =
            gtk_window_get_transient_for (
              GTK_WINDOW (self));
          ui_show_error_message (
            transient_parent, true, info->error_str);
        }
      return G_SOURCE_REMOVE;
    }
  else
    {
      return G_SOURCE_CONTINUE;
    }
}

/**
 * Adds a button at the start or end of the button box.
 *
 * @param start Whether to add the button at the
 *   start of the button box, otherwise the button
 *   will be added at the end.
 */
void
generic_progress_dialog_add_button (
  GenericProgressDialogWidget * self,
  GtkButton *                   btn,
  bool                          start,
  bool                          only_on_finish)
{
  GET_PRIVATE (self);

  GenericProgressDialogButton * extra_btn =
    &prv->extra_buttons[prv->num_extra_buttons++];

  /*bool expand = false;*/
  /*bool fill = true;*/
  /*guint padding = 0;*/
  if (start)
    {
      gtk_box_append (
        GTK_BOX (prv->action_btn_box),
        GTK_WIDGET (btn));
    }
  else
    {
      gtk_box_append (
        GTK_BOX (prv->action_btn_box),
        GTK_WIDGET (btn));
    }

  gtk_widget_set_visible (
    GTK_WIDGET (btn), !only_on_finish);

  extra_btn->btn = btn;
  extra_btn->only_on_finish = only_on_finish;
}

GenericProgressDialogWidget *
generic_progress_dialog_widget_new (void)
{
  return
    g_object_new (
      GENERIC_PROGRESS_DIALOG_WIDGET_TYPE, NULL);
}

/**
 * Sets up a progress dialog widget.
 */
void
generic_progress_dialog_widget_setup (
  GenericProgressDialogWidget * self,
  const char *                  title,
  GenericProgressInfo *         progress_info,
  bool                          autoclose,
  bool                          cancelable)
{
  gtk_window_set_title (GTK_WINDOW (self), title);

  GET_PRIVATE (self);
  prv->progress_info = progress_info;
  prv->autoclose = autoclose;

  gtk_label_set_text (
    prv->label, progress_info->label_str);

  if (cancelable)
    {
      gtk_widget_set_visible (
        GTK_WIDGET (prv->cancel), true);
    }

  gtk_widget_add_tick_callback (
    GTK_WIDGET (prv->progress_bar),
    (GtkTickCallback) tick_cb, self, NULL);

  g_signal_connect (
    G_OBJECT (self), "close",
    G_CALLBACK (on_closed), self);
}

static void
generic_progress_dialog_widget_class_init (
  GenericProgressDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "generic_progress_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child_private ( \
    klass, GenericProgressDialogWidget, x)

  BIND_CHILD (label);
  BIND_CHILD (ok);
  BIND_CHILD (cancel);
  BIND_CHILD (progress_bar);
  BIND_CHILD (action_btn_box);
  gtk_widget_class_bind_template_callback (
    klass, on_ok_clicked);
  gtk_widget_class_bind_template_callback (
    klass, on_cancel_clicked);

#undef BIND_CHILD
}

static void
generic_progress_dialog_widget_init (
  GenericProgressDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
