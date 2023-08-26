// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Generic dialog.
 */

#include <stdio.h>

#include "dsp/engine.h"
#include "gui/widgets/dialogs/generic_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/progress_info.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  GenericProgressDialogWidget,
  generic_progress_dialog_widget,
  GTK_TYPE_DIALOG)

#define GET_PRIVATE(x) \
  GenericProgressDialogWidgetPrivate * prv = \
    generic_progress_dialog_widget_get_instance_private (x)

static void
on_closed (
  GtkDialog *                   dialog,
  GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  progress_info_request_cancellation (prv->progress_info);
}

static void
on_ok_clicked (
  GtkButton *                   btn,
  GenericProgressDialogWidget * self)
{
  gtk_dialog_response (GTK_DIALOG (self), 0);
}

static void
on_cancel_clicked (
  GtkButton *                   btn,
  GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  progress_info_request_cancellation (prv->progress_info);
}

static gboolean
tick_cb (
  GtkWidget *                   widget,
  GdkFrameClock *               frame_clock,
  GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  ProgressInfo * info = prv->progress_info;

  double progress;
  progress_info_get_progress (info, &progress, NULL);
  gtk_progress_bar_set_fraction (
    GTK_PROGRESS_BAR (widget), progress);

  ProgressStatus status = progress_info_get_status (info);
  if (status == PROGRESS_STATUS_COMPLETED)
    {
      ProgressCompletionType compl_type =
        progress_info_get_completion_type (info);
      char * msg = progress_info_get_message (info);
      if (
        prv->autoclose
        || compl_type == PROGRESS_COMPLETED_CANCELLED)
        {
          gtk_dialog_response (GTK_DIALOG (self), 0);
        }
      else
        {
          gtk_widget_set_visible (GTK_WIDGET (prv->ok), true);
          gtk_widget_set_visible (
            GTK_WIDGET (prv->cancel), false);
          for (size_t i = 0; i < prv->num_extra_buttons; i++)
            {
              GenericProgressDialogButton * btn =
                &prv->extra_buttons[i];
              if (btn->only_on_finish)
                {
                  gtk_widget_set_visible (
                    GTK_WIDGET (btn->btn), true);
                }
            }
          gtk_label_set_text (prv->label, msg);
        }

      if (compl_type == PROGRESS_COMPLETED_HAS_ERROR)
        {
          GtkWindow * transient_parent =
            gtk_window_get_transient_for (GTK_WINDOW (self));
          ui_show_message_full (
            transient_parent, NULL, "%s", msg);
        }
      else if (msg)
        {
          GtkWindow * transient_parent =
            gtk_window_get_transient_for (GTK_WINDOW (self));
          ui_show_message_full (
            transient_parent, NULL, "%s", msg);
        }

      g_free (msg);

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
        GTK_BOX (prv->action_btn_box), GTK_WIDGET (btn));
    }
  else
    {
      gtk_box_append (
        GTK_BOX (prv->action_btn_box), GTK_WIDGET (btn));
    }

  gtk_widget_set_visible (GTK_WIDGET (btn), !only_on_finish);

  extra_btn->btn = btn;
  extra_btn->only_on_finish = only_on_finish;
}

GenericProgressDialogWidget *
generic_progress_dialog_widget_new (void)
{
  return g_object_new (
    GENERIC_PROGRESS_DIALOG_WIDGET_TYPE, NULL);
}

/**
 * Sets up a progress dialog widget.
 */
void
generic_progress_dialog_widget_setup (
  GenericProgressDialogWidget * self,
  const char *                  title,
  ProgressInfo *                progress_info,
  const char *                  initial_label,
  bool                          autoclose,
  bool                          cancelable)
{
  gtk_window_set_title (GTK_WINDOW (self), title);

  GET_PRIVATE (self);
  prv->progress_info = progress_info;
  prv->autoclose = autoclose;

  gtk_label_set_text (prv->label, initial_label);

  if (cancelable)
    {
      gtk_widget_set_visible (GTK_WIDGET (prv->cancel), true);
    }

  gtk_widget_add_tick_callback (
    GTK_WIDGET (prv->progress_bar), (GtkTickCallback) tick_cb,
    self, NULL);

  g_signal_connect (
    G_OBJECT (self), "close", G_CALLBACK (on_closed), self);
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
