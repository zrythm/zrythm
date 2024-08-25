// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdio>

#include "gui/widgets/dialogs/generic_progress_dialog.h"
#include "project.h"
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

auto get_private = [] (auto &self) {
  return static_cast<GenericProgressDialogWidgetPrivate *> (
    generic_progress_dialog_widget_get_instance_private (self));
};

static void
on_response_cb (
  AdwAlertDialog *              dialog,
  char *                        response,
  GenericProgressDialogWidget * self);

static void
run_callback_and_force_close (GenericProgressDialogWidget * self)
{
  auto prv = get_private (self);
  if (prv->close_cb.has_value ())
    {
      prv->close_cb.value ();
    }
  prv->close_cb.reset ();
  g_signal_handlers_disconnect_by_func (self, (gpointer) on_response_cb, self);
  adw_dialog_force_close (ADW_DIALOG (self));
}

static void
on_response_cb (
  AdwAlertDialog *              dialog,
  char *                        response,
  GenericProgressDialogWidget * self)
{
  z_debug ("generic progress response: {}", response);
  auto prv = get_private (self);
  if (string_is_equal (response, "cancel"))
    {
      z_debug ("accepting cancel response");
      prv->progress_info->request_cancellation ();
      run_callback_and_force_close (self);
    }
  else if (string_is_equal (response, "ok"))
    {
      /* nothing to do */
      z_debug ("accepting ok response");
      run_callback_and_force_close (self);
    }
  else
    {
      for (auto &btn : prv->extra_buttons)
        {
          if (response == btn.response)
            {
              /* call given callback */
              if (btn.cb.has_value ())
                {
                  btn.cb.value ();
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
  auto           prv = get_private (self);
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
          z_debug ("cancelled");
          /* dialog is already closed at this point (pressing cancel or closing
           * the window already invoked on_response()) */
        }
      else if (prv->autoclose)
        {
          z_debug ("autoclose");
          run_callback_and_force_close (self);
        }
      else
        {
          adw_alert_dialog_set_response_enabled (
            ADW_ALERT_DIALOG (self), "ok", true);
          adw_alert_dialog_set_response_enabled (
            ADW_ALERT_DIALOG (self), "cancel", false);
          for (auto &btn : prv->extra_buttons)
            {
              if (btn.only_on_finish)
                {
                  adw_alert_dialog_set_response_enabled (
                    ADW_ALERT_DIALOG (self), btn.response.c_str (), true);
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
  GenericProgressDialogWidget *  self,
  const char *                   response,
  const char *                   response_label,
  std::optional<GenericCallback> callback,
  bool                           only_on_finish)
{
  auto prv = get_private (self);

  GenericProgressDialogButton btn;

  adw_alert_dialog_add_response (
    ADW_ALERT_DIALOG (self), response, response_label);
  adw_alert_dialog_set_response_enabled (
    ADW_ALERT_DIALOG (self), response, !only_on_finish);

  btn.response = response;
  btn.cb = callback;
  btn.only_on_finish = only_on_finish;
  prv->extra_buttons.emplace_back (std::move (btn));
}

GenericProgressDialogWidget *
generic_progress_dialog_widget_new (void)
{
  return Z_GENERIC_PROGRESS_DIALOG_WIDGET (
    g_object_new (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE, nullptr));
}

/**
 * Sets up a progress dialog widget.
 */
void
generic_progress_dialog_widget_setup (
  GenericProgressDialogWidget *  self,
  const char *                   title,
  ProgressInfo *                 progress_info,
  const char *                   initial_label,
  bool                           autoclose,
  std::optional<GenericCallback> close_callback,
  bool                           cancelable)
{
  adw_alert_dialog_set_heading (ADW_ALERT_DIALOG (self), title);

  auto prv = get_private (self);
  prv->progress_info = progress_info;
  prv->autoclose = autoclose;
  prv->close_cb = close_callback;

  gtk_progress_bar_set_text (prv->progress_bar, initial_label);

  if (cancelable)
    {
      adw_alert_dialog_set_response_enabled (
        ADW_ALERT_DIALOG (self), "cancel", true);
    }

  gtk_widget_add_tick_callback (
    GTK_WIDGET (prv->progress_bar), (GtkTickCallback) tick_cb, self, nullptr);
}

static void
generic_progress_dialog_widget_finalize (GObject * object)
{
  auto self = Z_GENERIC_PROGRESS_DIALOG_WIDGET (object);
  auto prv = get_private (self);
  std::destroy_at (&prv->extra_buttons);
  std::destroy_at (&prv->close_cb);

  G_OBJECT_CLASS (generic_progress_dialog_widget_parent_class)->finalize (object);
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

  auto oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = generic_progress_dialog_widget_finalize;
}

static void
generic_progress_dialog_widget_init (GenericProgressDialogWidget * self)
{
  auto prv = get_private (self);
  std::construct_at (&prv->close_cb);
  std::construct_at (&prv->extra_buttons);

  gtk_widget_init_template (GTK_WIDGET (self));
}
