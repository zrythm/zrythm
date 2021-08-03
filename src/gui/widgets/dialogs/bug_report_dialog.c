/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include "actions/undo_manager.h"
#include "actions/undo_stack.h"
#include "gui/widgets/dialogs/bug_report_dialog.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/log.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  BugReportDialogWidget,
  bug_report_dialog_widget,
  GTK_TYPE_DIALOG)

static char *
get_report_template (
  BugReportDialogWidget * self,
  bool                    for_uri)
{
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_start_iter (
    GTK_TEXT_BUFFER (
      self->steps_to_reproduce_buffer),
    &start_iter);
  gtk_text_buffer_get_end_iter (
    GTK_TEXT_BUFFER (
      self->steps_to_reproduce_buffer),
    &end_iter);
  char * steps_to_reproduce =
    gtk_text_buffer_get_text (
      GTK_TEXT_BUFFER (
        self->steps_to_reproduce_buffer),
      &start_iter, &end_iter, false);

  gtk_text_buffer_get_start_iter (
    GTK_TEXT_BUFFER (
      self->other_info_buffer),
    &start_iter);
  gtk_text_buffer_get_end_iter (
    GTK_TEXT_BUFFER (
      self->other_info_buffer),
    &end_iter);
  char * other_info =
    gtk_text_buffer_get_text (
      GTK_TEXT_BUFFER (
        self->other_info_buffer),
      &start_iter, &end_iter, false);

  /* %23 is hash, %0A is new line */
  char ver_with_caps[2000];
  zrythm_get_version_with_capabilities (
    ver_with_caps);
  char * report_template =
    g_strdup_printf (
      "# What did you do?\n"
      "%s\n\n"
      "# Version\n```\n%s```\n\n"
      "# Other info\n"
      "%s\n\n"
      "# Backtrace\n```\n%s```\n\n"
      "# Action stack\n```\n%s```\n\n"
      "# Log\n```\n%s```",
      steps_to_reproduce,
      ver_with_caps, other_info,
      self->backtrace,
      self->undo_stack, self->log);

  char * ret;
  if (for_uri)
    ret =
      g_uri_escape_string (
        report_template, NULL, FALSE);
  else
    ret =
      g_markup_escape_text (report_template, -1);

  g_free (steps_to_reproduce);
  g_free (other_info);
  g_free (report_template);

  return ret;
}

static void
on_button_close_clicked (
  GtkButton *             btn,
  BugReportDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self), GTK_RESPONSE_CLOSE);
}

static void
on_button_send_srht_clicked (
  GtkButton *             btn,
  BugReportDialogWidget * self)
{
  /* create new dialog */
  GtkDialogFlags flags =
    GTK_DIALOG_MODAL
    | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog =
    gtk_message_dialog_new_with_markup (
      GTK_WINDOW (self), flags,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      NULL);

  gtk_window_set_title (
    GTK_WINDOW (dialog), _("Send via Sourcehut"));

  /* set top text */
  char * atag =
    g_strdup_printf (
      "<a href=\"%s\">",
      NEW_ISSUE_URL);
  char * markup =
    g_strdup_printf (
      _("Please copy the template below in a "
        "%snew issue%s."),
      atag, "</a>");
  gtk_message_dialog_set_markup (
    GTK_MESSAGE_DIALOG (dialog), markup);
  g_free (atag);
  g_free (markup);

  /* set bottom text - template */
  char * report_template =
    get_report_template (self, false);
  gtk_message_dialog_format_secondary_markup (
    GTK_MESSAGE_DIALOG (dialog),
    "%s", report_template);
  g_free (report_template);

  /* make label selectable */
  GtkLabel * label =
    z_gtk_message_dialog_get_label (
      GTK_MESSAGE_DIALOG (dialog), 1);
  gtk_label_set_selectable (label, true);

  /* wrap bottom text in a scrolled window */
  z_gtk_message_dialog_wrap_message_area_in_scroll (
    GTK_MESSAGE_DIALOG (dialog), 580, 360);

  /* run the dialog */
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (
    GTK_WIDGET (dialog));
}

static void
on_button_send_email_clicked (
  GtkButton *             btn,
  BugReportDialogWidget * self)
{
  char * report_template =
    get_report_template (self, true);
  char * email_url =
    g_strdup_printf (
      "mailto:%s?body=%s",
      NEW_ISSUE_EMAIL, report_template);
  g_free (report_template);

  GError * err = NULL;
  bool success =
    gtk_show_uri_on_window (
      GTK_WINDOW (self), email_url,
      GDK_CURRENT_TIME, &err);
  g_free (email_url);
  if (!success)
    {
      g_warning (
        "failed to show URI on window: %s",
        err->message);
    }
}

static void
on_button_send_automatically_clicked (
  GtkButton *             btn,
  BugReportDialogWidget * self)
{
  /* TODO */
}

static void
setup_text_view (
  GtkSourceView * text_view)
{
  /* set text language */
  GtkSourceLanguageManager * manager =
    z_gtk_source_language_manager_get ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (
      manager, "markdown");
  GtkSourceBuffer * buffer =
    GTK_SOURCE_BUFFER (
      gtk_text_view_get_buffer (
        GTK_TEXT_VIEW (text_view)));
  gtk_source_buffer_set_language (buffer, lang);

/*#if 0*/
  /* set style */
  GtkSourceStyleSchemeManager * style_mgr =
    gtk_source_style_scheme_manager_get_default ();
  gtk_source_style_scheme_manager_prepend_search_path (
    style_mgr, CONFIGURE_SOURCEVIEW_STYLES_DIR);
  gtk_source_style_scheme_manager_force_rescan (
    style_mgr);
  GtkSourceStyleScheme * scheme =
    gtk_source_style_scheme_manager_get_scheme (
      style_mgr, "monokai-extended-zrythm");
  gtk_source_buffer_set_style_scheme (
    buffer, scheme);
/*#endif*/
}

/**
 * Creates and displays the about dialog.
 */
BugReportDialogWidget *
bug_report_dialog_new (
  GtkWindow *  parent,
  const char * msg_prefix,
  const char * backtrace)
{
  BugReportDialogWidget * self =
    g_object_new (
      BUG_REPORT_DIALOG_WIDGET_TYPE, NULL);

  char * markup =
    g_strdup_printf (
      _("%sPlease help us fix this by "
        "submitting a bug report."),
      msg_prefix);
  gtk_label_set_markup (
    self->top_lbl, markup);

  self->log =
    log_get_last_n_lines (LOG, 60);
  self->log_long =
    log_get_last_n_lines (LOG, 200);
  self->undo_stack =
    PROJECT && UNDO_MANAGER ?
      undo_stack_get_as_string (
        UNDO_MANAGER->undo_stack, 12) :
      g_strdup ("<undo stack uninitialized>");
  self->undo_stack_long =
    PROJECT && UNDO_MANAGER ?
      undo_stack_get_as_string (
        UNDO_MANAGER->undo_stack, 64) :
      g_strdup ("<undo stack uninitialized>");
  self->backtrace = g_strdup (backtrace);

  setup_text_view (
    self->steps_to_reproduce_text_view);
  setup_text_view (
    self->other_info_text_view);

  gtk_widget_set_tooltip_text (
    GTK_WIDGET (
      self->steps_to_reproduce_text_view),
    _("Enter a list of steps to reproduce the "
    "error. Markdown is supported."));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (
      self->other_info_text_view),
    _("Enter more context, what distro you use, "
    "etc. Markdown is supported."));

  return self;
}

static void
bug_report_dialog_widget_class_init (
  BugReportDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "bug_report_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, BugReportDialogWidget, x)

  BIND_CHILD (top_lbl);
  BIND_CHILD (steps_to_reproduce_text_view);
  BIND_CHILD (other_info_text_view);
  BIND_CHILD (steps_to_reproduce_buffer);
  BIND_CHILD (other_info_buffer);

#undef BIND_CHILD

#define BIND_CALLBACK(x) \
  gtk_widget_class_bind_template_callback ( \
    klass, x);

  BIND_CALLBACK (on_button_close_clicked);
  BIND_CALLBACK (on_button_send_srht_clicked);
  BIND_CALLBACK (on_button_send_email_clicked);
  BIND_CALLBACK (on_button_send_automatically_clicked);

#undef BIND_CALLBACK
}

static void
bug_report_dialog_widget_init (
  BugReportDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
