/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/dialogs/generic_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/curl.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <json-glib/json-glib.h>

G_DEFINE_TYPE (
  BugReportDialogWidget,
  bug_report_dialog_widget,
  GTK_TYPE_DIALOG)

static bool
validate_input (
  BugReportDialogWidget * self)
{
  char * steps_to_reproduce =
    z_gtk_text_buffer_get_full_text (
      GTK_TEXT_BUFFER (
        self->steps_to_reproduce_buffer));
  if (string_is_empty (steps_to_reproduce))
    {
      g_free_and_null (steps_to_reproduce);
      ui_show_error_message (
        MAIN_WINDOW, false,
        _("Please enter steps to reproduce"));
      return false;
    }

  char * other_info =
    z_gtk_text_buffer_get_full_text (
      GTK_TEXT_BUFFER (
        self->other_info_buffer));
  if (string_is_empty (other_info))
    {
      g_free_and_null (other_info);
      ui_show_error_message (
        MAIN_WINDOW, false,
        _("Please fill in all fields"));
      return false;
    }

  return true;
}

static char *
get_report_template (
  BugReportDialogWidget * self,
  bool                    for_uri)
{
  char * steps_to_reproduce =
    z_gtk_text_buffer_get_full_text (
      GTK_TEXT_BUFFER (
        self->steps_to_reproduce_buffer));
  char * other_info =
    z_gtk_text_buffer_get_full_text (
      GTK_TEXT_BUFFER (
        self->other_info_buffer));

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
      "# Fatal\n%s\n\n"
      "# Action stack\n```\n%s```\n\n"
      "# Log\n```\n%s```",
      steps_to_reproduce,
      ver_with_caps, other_info,
      self->backtrace,
      self->fatal ? "Yes" : "No",
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
  if (!validate_input (self))
    return;

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
    GTK_MESSAGE_DIALOG (dialog), 580, 320);

  /* run the dialog */
  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

static void
on_button_send_email_clicked (
  GtkButton *             btn,
  BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  char * report_template =
    get_report_template (self, true);
  char * email_url =
    g_strdup_printf (
      "mailto:%s?body=%s",
      NEW_ISSUE_EMAIL, report_template);
  g_free (report_template);

  gtk_show_uri (
    GTK_WINDOW (self), email_url,
    GDK_CURRENT_TIME);
  g_free (email_url);
}

static char *
get_json_string (
  BugReportDialogWidget * self)
{
  g_debug ("%s: generating json...", __func__);

  JsonBuilder *builder = json_builder_new ();

  json_builder_begin_object (builder);

  char * steps_to_reproduce =
    z_gtk_text_buffer_get_full_text (
      GTK_TEXT_BUFFER (
        self->steps_to_reproduce_buffer));
  json_builder_set_member_name (
    builder, "steps_to_reproduce");
  json_builder_add_string_value (
    builder, steps_to_reproduce);
  g_free (steps_to_reproduce);

  char * other_info =
    z_gtk_text_buffer_get_full_text (
      GTK_TEXT_BUFFER (
        self->other_info_buffer));
  json_builder_set_member_name (
    builder, "extra_info");
  json_builder_add_string_value (
    builder, other_info);
  g_free (other_info);

  json_builder_set_member_name (
    builder, "action_stack");
  json_builder_add_string_value (
    builder, self->undo_stack_long);

  json_builder_set_member_name (
    builder, "backtrace");
  json_builder_add_string_value (
    builder, self->backtrace);

#if 0
  json_builder_set_member_name (
    builder, "log");
  json_builder_add_string_value (
    builder, self->log_long);
#endif

  json_builder_set_member_name (
    builder, "fatal");
  json_builder_add_int_value (
    builder, self->fatal);

  char ver_with_caps[2000];
  zrythm_get_version_with_capabilities (
    ver_with_caps);
  json_builder_set_member_name (
    builder, "version");
  json_builder_add_string_value (
    builder, ver_with_caps);

  json_builder_end_object (builder);

  JsonGenerator *gen = json_generator_new ();
  JsonNode * root = json_builder_get_root (builder);
  json_generator_set_root (gen, root);
  gchar *str = json_generator_to_data (gen, NULL);

  json_node_free (root);
  g_object_unref (gen);
  g_object_unref (builder);

  return str;
}

typedef struct AutomaticReportData
{
  const char * json;
  const char * screenshot_path;
  const char * log_file_path;
  GenericProgressInfo progress_nfo;
} AutomaticReportData;

static void *
send_data (
  AutomaticReportData * data)
{
  GError * err = NULL;
  int ret =
    z_curl_post_json_no_auth (
      BUG_REPORT_API_ENDPOINT, data->json,
      7, &err,
      /* screenshot */
      "screenshot", data->screenshot_path,
      "image/jpeg",
      /* log */
      "log_file", data->log_file_path,
      "application/zstd",
      NULL);
  if (ret != 0)
    {
      data->progress_nfo.has_error = true;
      sprintf (
        data->progress_nfo.error_str,
        _("error: %s"), err->message);
    }

  data->progress_nfo.progress = 1.0;
  strcpy (
    data->progress_nfo.label_done_str, _("Done"));

  return NULL;
}

static void
on_button_send_automatically_clicked (
  GtkButton *             btn,
  BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  /* create new dialog */
  GtkDialogFlags flags =
    GTK_DIALOG_MODAL
    | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog =
    gtk_dialog_new_with_buttons (
      _("Send Automatically"),
      GTK_WINDOW (self), flags,
      _("_OK"),
      GTK_RESPONSE_OK,
      _("_Cancel"),
      GTK_RESPONSE_CANCEL,
      NULL);

  GtkWidget * content_area =
    gtk_dialog_get_content_area (
      GTK_DIALOG (dialog));
  GtkGrid * grid = GTK_GRID (gtk_grid_new ());
  z_gtk_widget_set_margin (GTK_WIDGET (grid), 4);
  gtk_grid_set_row_spacing (grid, 2);
  gtk_box_append (
    GTK_BOX (content_area),
    GTK_WIDGET (grid));

  /* set top text */
  char * atag =
    g_strdup_printf (
      "<a href=\"%s\">",
      PRIVACY_POLICY_URL);
  char * markup =
    g_strdup_printf (
      _("Click OK to submit. You can verify "
      "what will be sent below. "
      "By clicking OK, you agree to our "
      "%sPrivacy Policy%s."),
      atag, "</a>");
  GtkWidget * top_label = gtk_label_new ("");
  gtk_label_set_markup (
    GTK_LABEL (top_label), markup);
  gtk_grid_attach (
    grid, top_label, 0, 0, 1, 1);
  g_free (atag);
  g_free (markup);

  /* set json */
  GtkWidget * json_heading = gtk_label_new ("");
  gtk_label_set_markup (
    GTK_LABEL (json_heading),
    _("<b>Data</b>"));
  gtk_grid_attach (
    grid, json_heading, 0, 1, 1, 1);
  char * json_str = get_json_string (self);
  GtkWidget * json_label = gtk_source_view_new ();
  gtk_text_view_set_wrap_mode (
    GTK_TEXT_VIEW (json_label), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_editable (
    GTK_TEXT_VIEW (json_label), false);
  GtkTextBuffer * json_label_buf =
    gtk_text_view_get_buffer (
      GTK_TEXT_VIEW (json_label));
  GtkSourceLanguageManager * manager =
    z_gtk_source_language_manager_get ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (
      manager, "json");
  gtk_source_buffer_set_language (
    GTK_SOURCE_BUFFER (json_label_buf), lang);
  gtk_text_buffer_set_text (
    json_label_buf, json_str, -1);
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
    GTK_SOURCE_BUFFER (json_label_buf), scheme);
  GtkWidget * scrolled_window =
    gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scrolled_window),
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_width (
    GTK_SCROLLED_WINDOW (scrolled_window),
    580);
  gtk_scrolled_window_set_min_content_height (
    GTK_SCROLLED_WINDOW (scrolled_window),
    340);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scrolled_window),
    json_label);
  gtk_grid_attach (
    grid, scrolled_window, 0, 2, 1, 1);

  /* log file */
  GtkWidget * log_file_heading =
    gtk_label_new ("");
  gtk_label_set_markup (
    GTK_LABEL (log_file_heading),
    _("<b>Log file</b>"));
  gtk_grid_attach (
    grid, log_file_heading, 0, 3, 1, 1);
  char * log_file_tmpdir = NULL;
  char * log_file_path = NULL;
  if (LOG && LOG->log_filepath)
    {
      /* create a zstd-compressed log file */
      GError * err = NULL;
      bool ret =
        log_generate_compressed_file (
          LOG, &log_file_tmpdir, &log_file_path,
          &err);
      if (!ret)
        {
          g_warning (
            "could not create tmp dir for log "
            "file: %s", err->message);
          g_error_free (err);
        }
    }
  GtkWidget * log_file_path_lbl =
    gtk_label_new (NULL);
  if (log_file_path)
    {
      char * str =
        g_strdup_printf (
          "<a href=\"file://%s\">%s</a>",
          log_file_path, log_file_path);
      gtk_label_set_markup (
        GTK_LABEL (log_file_path_lbl), str);
      g_free (str);
    }
  gtk_grid_attach (
    grid, log_file_path_lbl, 0, 4, 1, 1);

  /* screenshot */
  GtkWidget * screenshot_heading =
    gtk_label_new ("");
  gtk_label_set_markup (
    GTK_LABEL (screenshot_heading),
    _("<b>Screenshot</b>"));
  gtk_grid_attach (
    grid, screenshot_heading, 0, 5, 1, 1);
  char * screenshot_tmpdir = NULL;
  char * screenshot_path = NULL;
  if (MAIN_WINDOW)
    {
      const char * option_keys[] = {
        "quality", NULL, };
      const char * option_vals[] = {
        "30", NULL, };
      z_gtk_generate_screenshot_image (
        GTK_WIDGET (MAIN_WINDOW),
        "jpeg",
        (char **) option_keys, (char **) option_vals,
        &screenshot_tmpdir,
        &screenshot_path, true);
    }
  GtkWidget * img = gtk_image_new ();
  if (screenshot_path)
    {
      GError * err = NULL;
      GdkPixbuf * pixbuf =
        gdk_pixbuf_new_from_file_at_scale (
          screenshot_path, 580, 360, true, &err);
      gtk_widget_set_size_request (
        GTK_WIDGET (img), 580, 360);
      if (pixbuf)
        {
          gtk_image_set_from_pixbuf (
            GTK_IMAGE (img), pixbuf);
          g_object_unref (pixbuf);
        }
      else
        {
          g_warning (
            "could not get pixbuf from %s: %s",
            screenshot_path, err->message);
        }
    }
  gtk_grid_attach (
    grid, img, 0, 6, 1, 1);

  /* run the dialog */
  int ret =
    z_gtk_dialog_run (GTK_DIALOG (dialog), true);

  if (ret != GTK_RESPONSE_OK)
    return;

  char message[6000];
  strcpy (message, _("Sending"));

  AutomaticReportData data;
  memset (&data, 0, sizeof (AutomaticReportData));
  data.json = json_str;
  data.screenshot_path = screenshot_path;
  data.log_file_path = log_file_path;
  data.progress_nfo.progress = 0;
  strcpy (
    data.progress_nfo.label_str, _("Sending data..."));
  strcpy (
    data.progress_nfo.label_done_str, _("Done"));
  strcpy (data.progress_nfo.error_str, _("Failed"));
  GenericProgressDialogWidget * progress_dialog =
    generic_progress_dialog_widget_new ();
  generic_progress_dialog_widget_setup (
    progress_dialog, _("Sending..."),
    &data.progress_nfo,
    true, false);

  /* start sending in a new thread */
  GThread * thread =
    g_thread_new (
      "bounce_thread",
      (GThreadFunc) send_data,
      &data);

  /* run dialog */
  gtk_window_set_transient_for (
    GTK_WINDOW (progress_dialog),
    GTK_WINDOW (self));
  z_gtk_dialog_run (
    GTK_DIALOG (progress_dialog), true);

  g_thread_join (thread);

  if (!data.progress_nfo.has_error)
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->button_send_automatically),
        false);

      ui_show_message_printf (
        self, GTK_MESSAGE_INFO, false, "%s",
        _("Sent successfully"));
    }

  if (log_file_path)
    {
      io_remove (log_file_path);
      g_free_and_null (log_file_path);
    }
  if (log_file_tmpdir)
    {
      io_rmdir (log_file_tmpdir, Z_F_NO_FORCE);
      g_free_and_null (log_file_tmpdir);
    }
  if (screenshot_path)
    {
      io_remove (screenshot_path);
      g_free_and_null (screenshot_path);
    }
  if (screenshot_tmpdir)
    {
      io_rmdir (screenshot_tmpdir, Z_F_NO_FORCE);
      g_free_and_null (screenshot_tmpdir);
    }

  g_free (json_str);
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
}

/**
 * Creates and displays the about dialog.
 */
BugReportDialogWidget *
bug_report_dialog_new (
  GtkWindow *  parent,
  const char * msg_prefix,
  const char * backtrace,
  bool         fatal)
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
    log_get_last_n_lines (LOG, 160);
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
  self->fatal = fatal;

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
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "bug_report_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, BugReportDialogWidget, x)

  BIND_CHILD (top_lbl);
  BIND_CHILD (button_send_automatically);
  BIND_CHILD (steps_to_reproduce_text_view);
  BIND_CHILD (other_info_text_view);
  BIND_CHILD (steps_to_reproduce_buffer);
  BIND_CHILD (other_info_buffer);
  BIND_CHILD (button_close);
  BIND_CHILD (button_send_srht);
  BIND_CHILD (button_send_email);
  BIND_CHILD (button_send_automatically);

#undef BIND_CHILD
}

static void
bug_report_dialog_widget_init (
  BugReportDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->button_close), "clicked",
    G_CALLBACK (on_button_close_clicked), self);
  g_signal_connect (
    G_OBJECT (self->button_send_srht), "clicked",
    G_CALLBACK (on_button_send_srht_clicked), self);
  g_signal_connect (
    G_OBJECT (self->button_send_email), "clicked",
    G_CALLBACK (on_button_send_email_clicked),
    self);
  g_signal_connect (
    G_OBJECT (self->button_send_automatically),
    "clicked",
    G_CALLBACK (
      on_button_send_automatically_clicked), self);
}
