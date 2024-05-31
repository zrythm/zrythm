// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Generic dialog.
 */

#include <cstdio>

#include "dsp/engine.h"
#include "gui/widgets/dialogs/generic_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/progress_info.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE_WITH_PRIVATE (
  GenericProgressDialogWidget,
  generic_progress_dialog_widget,
  ADW_TYPE_ALERT_DIALOG)

#define GET_PRIVATE(x) \
  GenericProgressDialogWidgetPrivate * prv = \
    static_cast<GenericProgressDialogWidgetPrivate *> ( \
      generic_progress_dialog_widget_get_instance_private (x))

static void
on_response_cb (
  AdwAlertDialog *              dialog,
  char *                        response,
  GenericProgressDialogWidget * self);

static void
run_callback_and_force_close (GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  if (prv->close_cb)
    {
      prv->close_cb (prv->close_cb_obj);
    }
  prv->close_cb = NULL;
  g_signal_handlers_disconnect_by_func (self, (gpointer) on_response_cb, self);
  adw_dialog_force_close (ADW_DIALOG (self));
}

static void
on_response_cb (
  AdwAlertDialog *              dialog,
  char *                        response,
  GenericProgressDialogWidget * self)
{
  g_debug ("generic progress response: %s", response);
  GET_PRIVATE (self);
  if (string_is_equal (response, "cancel"))
    {
      g_debug ("accepting cancel response");
      prv->progress_info->request_cancellation ();
      run_callback_and_force_close (self);
    }
  else if (string_is_equal (response, "ok"))
    {
      /* nothing to do */
      g_debug ("accepting ok response");
      run_callback_and_force_close (self);
    }
  else
    {
      for (size_t i = 0; i < prv->num_extra_buttons; i++)
        {
          GenericProgressDialogButton * btn = &prv->extra_buttons[i];
          if (string_is_equal (response, btn->response))
            {
              /* call given callback */
              if (btn->cb)
                {
                  btn->cb (btn->cb_obj);
                }
              break;
            }
        }
    }
}

static gboolean
tick_cb (
  GtkWidget *                   widget,
  GdkFrameClock *               frame_clock,
  GenericProgressDialogWidget * self)
{
  GET_PRIVATE (self);
  ProgressInfo * info = prv->progress_info;

  double      progress;
  std::string progress_str;
  std::tie (progress, progress_str) = info->get_progress ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), progress);

  ProgressInfo::Status status = info->get_status ();
  if (status == ProgressInfo::Status::COMPLETED)
    {
      ProgressInfo::CompletionType compl_type = info->get_completion_type ();
      char *                       msg = info->get_message ();
      if (compl_type == ProgressInfo::CompletionType::CANCELLED)
        {
          g_debug ("cancelled");
          /* dialog is already closed at this point (pressing cancel or closing
           * the window already invoked on_response()) */
        }
      else if (prv->autoclose)
        {
          g_debug ("autoclose");
          run_callback_and_force_close (self);
        }
      else
        {
          adw_alert_dialog_set_response_enabled (
            ADW_ALERT_DIALOG (self), "ok", true);
          adw_alert_dialog_set_response_enabled (
            ADW_ALERT_DIALOG (self), "cancel", false);
          for (size_t i = 0; i < prv->num_extra_buttons; i++)
            {
              GenericProgressDialogButton * btn = &prv->extra_buttons[i];
              if (btn->only_on_finish)
                {
                  adw_alert_dialog_set_response_enabled (
                    ADW_ALERT_DIALOG (self), btn->response, true);
                }
            }
          gtk_progress_bar_set_text (prv->progress_bar, msg);
        }

      if (compl_type == ProgressInfo::CompletionType::HAS_ERROR)
        {
          ui_show_message_full (GTK_WIDGET (self), _ ("Error"), "%s", msg);
        }
      else if (msg)
        {
          ui_show_notification (msg);
        }

      g_free (msg);

      return G_SOURCE_REMOVE;
    }
  else
    {
      return G_SOURCE_CONTINUE;
    }
}

void
generic_progress_dialog_add_response (
  GenericProgressDialogWidget * self,
  const char *                  response,
  const char *                  response_label,
  GenericCallback               callback,
  void *                        callback_object,
  bool                          only_on_finish)
{
  GET_PRIVATE (self);

  GenericProgressDialogButton * extra_btn =
    &prv->extra_buttons[prv->num_extra_buttons++];

  adw_alert_dialog_add_response (
    ADW_ALERT_DIALOG (self), response, response_label);
  adw_alert_dialog_set_response_enabled (
    ADW_ALERT_DIALOG (self), response, !only_on_finish);

  strncpy (extra_btn->response, response, 200);
  extra_btn->cb = callback;
  extra_btn->cb_obj = callback_object;
  extra_btn->only_on_finish = only_on_finish;
}

GenericProgressDialogWidget *
generic_progress_dialog_widget_new (void)
{
  return Z_GENERIC_PROGRESS_DIALOG_WIDGET (
    g_object_new (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE, NULL));
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
  GenericCallback               close_callback,
  void *                        close_callback_object,
  bool                          cancelable)
{
  adw_alert_dialog_set_heading (ADW_ALERT_DIALOG (self), title);

  GET_PRIVATE (self);
  prv->progress_info = progress_info;
  prv->autoclose = autoclose;
  prv->close_cb = close_callback;
  prv->close_cb_obj = close_callback_object;

  gtk_progress_bar_set_text (prv->progress_bar, initial_label);

  if (cancelable)
    {
      adw_alert_dialog_set_response_enabled (
        ADW_ALERT_DIALOG (self), "cancel", true);
    }

  gtk_widget_add_tick_callback (
    GTK_WIDGET (prv->progress_bar), (GtkTickCallback) tick_cb, self, NULL);
}

static void
generic_progress_dialog_widget_class_init (
  GenericProgressDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "generic_progress_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child_private ( \
    klass, GenericProgressDialogWidget, x)

  BIND_CHILD (progress_bar);
  gtk_widget_class_bind_template_callback (klass, on_response_cb);

#undef BIND_CHILD
}

static void
generic_progress_dialog_widget_init (GenericProgressDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
