// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
#include "utils/progress_info.h"
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

#define SOURCEHUT_RESPONSE 450
#define EMAIL_RESPONSE 451
#define PREVIEW_AND_SEND_AUTOMATICALLY_RESPONSE 452

static bool
validate_input (BugReportDialogWidget * self)
{
  char * steps_to_reproduce = z_gtk_text_buffer_get_full_text (
    gtk_text_view_get_buffer (self->user_input_text_view));
  if (string_is_empty (steps_to_reproduce))
    {
      g_free_and_null (steps_to_reproduce);
      ui_show_error_message (
        false, _ ("Please enter more details"));
      return false;
    }

  return true;
}

static char *
get_report_template (BugReportDialogWidget * self, bool for_uri)
{
  char * steps_to_reproduce = z_gtk_text_buffer_get_full_text (
    gtk_text_view_get_buffer (self->user_input_text_view));

  /* %23 is hash, %0A is new line */
  char ver_with_caps[2000];
  zrythm_get_version_with_capabilities (ver_with_caps, false);
  char * report_template = g_strdup_printf (
    "# What did you do?\n"
    "%s\n\n"
    "# Version\n```\n%s```\n\n"
    "# System info\n```\n%s\n```\n\n"
    "# Backtrace\n```\n%s```\n\n"
    "# Fatal\n%s\n\n"
    "# Action stack\n```\n%s```\n\n"
    "# Log\n```\n%s```",
    steps_to_reproduce, ver_with_caps, self->system_nfo,
    self->backtrace, self->fatal ? "Yes" : "No",
    self->undo_stack, self->log);

  char * ret;
  if (for_uri)
    ret = g_uri_escape_string (report_template, NULL, FALSE);
  else
    ret = g_markup_escape_text (report_template, -1);

  g_free (steps_to_reproduce);
  g_free (report_template);

  return ret;
}

static char *
get_json_string (BugReportDialogWidget * self)
{
  g_debug ("%s: generating json...", __func__);

  JsonBuilder * builder = json_builder_new ();

  json_builder_begin_object (builder);

  char * steps_to_reproduce = z_gtk_text_buffer_get_full_text (
    gtk_text_view_get_buffer (self->user_input_text_view));
  json_builder_set_member_name (builder, "steps_to_reproduce");
  json_builder_add_string_value (builder, steps_to_reproduce);
  g_free (steps_to_reproduce);

  json_builder_set_member_name (builder, "extra_info");
  json_builder_add_string_value (builder, self->system_nfo);

  json_builder_set_member_name (builder, "action_stack");
  json_builder_add_string_value (
    builder, self->undo_stack_long);

  json_builder_set_member_name (builder, "backtrace");
  json_builder_add_string_value (builder, self->backtrace);

#if 0
  json_builder_set_member_name (
    builder, "log");
  json_builder_add_string_value (
    builder, self->log_long);
#endif

  json_builder_set_member_name (builder, "fatal");
  json_builder_add_int_value (builder, self->fatal);

  char ver_with_caps[2000];
  zrythm_get_version_with_capabilities (ver_with_caps, false);
  json_builder_set_member_name (builder, "version");
  json_builder_add_string_value (builder, ver_with_caps);

  json_builder_end_object (builder);

  JsonGenerator * gen = json_generator_new ();
  JsonNode *      root = json_builder_get_root (builder);
  json_generator_set_root (gen, root);
  gchar * str = json_generator_to_data (gen, NULL);

  json_node_free (root);
  g_object_unref (gen);
  g_object_unref (builder);

  return str;
}

typedef struct AutomaticReportData
{
  const char *   json;
  const char *   screenshot_path;
  const char *   log_file_path;
  ProgressInfo * progress_nfo;
} AutomaticReportData;

static void *
send_data (AutomaticReportData * data)
{
  GError * err = NULL;
  int      ret = z_curl_post_json_no_auth (
         BUG_REPORT_API_ENDPOINT, data->json, 7, &err,
         /* screenshot */
         "screenshot", data->screenshot_path, "image/jpeg",
         /* log */
         "log_file", data->log_file_path, "application/zstd", NULL);
  if (ret != 0)
    {
      progress_info_mark_completed (
        data->progress_nfo, PROGRESS_COMPLETED_HAS_ERROR,
        err->message);
      return NULL;
    }

  progress_info_mark_completed (
    data->progress_nfo, PROGRESS_COMPLETED_SUCCESS,
    _ ("Done"));

  return NULL;
}

static void
on_preview_and_send_automatically_response (
  BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  /* create new dialog */
  GtkDialogFlags flags =
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog = gtk_dialog_new_with_buttons (
    _ ("Send Automatically"), GTK_WINDOW (self), flags,
    _ ("_OK"), GTK_RESPONSE_OK, _ ("_Cancel"),
    GTK_RESPONSE_CANCEL, NULL);

  GtkWidget * content_area =
    gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkGrid * grid = GTK_GRID (gtk_grid_new ());
  z_gtk_widget_set_margin (GTK_WIDGET (grid), 4);
  gtk_grid_set_row_spacing (grid, 2);
  gtk_box_append (GTK_BOX (content_area), GTK_WIDGET (grid));

  /* set top text */
  char * atag =
    g_strdup_printf ("<a href=\"%s\">", PRIVACY_POLICY_URL);
  char * markup = g_strdup_printf (
    _ ("Click OK to submit. You can verify "
       "what will be sent below. "
       "By clicking OK, you agree to our "
       "%sPrivacy Policy%s."),
    atag, "</a>");
  GtkWidget * top_label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (top_label), markup);
  gtk_grid_attach (grid, top_label, 0, 0, 1, 1);
  g_free (atag);
  g_free (markup);

  /* set json */
  GtkWidget * json_heading = gtk_label_new ("");
  gtk_label_set_markup (
    GTK_LABEL (json_heading), _ ("<b>Data</b>"));
  gtk_grid_attach (grid, json_heading, 0, 1, 1, 1);
  char *      json_str = get_json_string (self);
  GtkWidget * json_label = gtk_source_view_new ();
  gtk_text_view_set_wrap_mode (
    GTK_TEXT_VIEW (json_label), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_editable (
    GTK_TEXT_VIEW (json_label), false);
  GtkTextBuffer * json_label_buf =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (json_label));
  GtkSourceLanguageManager * manager =
    z_gtk_source_language_manager_get ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (manager, "json");
  gtk_source_buffer_set_language (
    GTK_SOURCE_BUFFER (json_label_buf), lang);
  gtk_text_buffer_set_text (json_label_buf, json_str, -1);
  GtkSourceStyleSchemeManager * style_mgr =
    gtk_source_style_scheme_manager_get_default ();
  gtk_source_style_scheme_manager_prepend_search_path (
    style_mgr, CONFIGURE_SOURCEVIEW_STYLES_DIR);
  gtk_source_style_scheme_manager_force_rescan (style_mgr);
  GtkSourceStyleScheme * scheme =
    gtk_source_style_scheme_manager_get_scheme (
      style_mgr, "monokai-extended-zrythm");
  gtk_source_buffer_set_style_scheme (
    GTK_SOURCE_BUFFER (json_label_buf), scheme);
  GtkWidget * scrolled_window = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER,
    GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_width (
    GTK_SCROLLED_WINDOW (scrolled_window), 580);
  gtk_scrolled_window_set_min_content_height (
    GTK_SCROLLED_WINDOW (scrolled_window), 340);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scrolled_window), json_label);
  gtk_grid_attach (grid, scrolled_window, 0, 2, 1, 1);

  /* log file */
  GtkWidget * log_file_heading = gtk_label_new ("");
  gtk_label_set_markup (
    GTK_LABEL (log_file_heading), _ ("<b>Log file</b>"));
  gtk_grid_attach (grid, log_file_heading, 0, 3, 1, 1);
  char * log_file_tmpdir = NULL;
  char * log_file_path = NULL;
  if (LOG && LOG->log_filepath)
    {
      /* create a zstd-compressed log file */
      GError * err = NULL;
      bool     ret = log_generate_compressed_file (
            LOG, &log_file_tmpdir, &log_file_path, &err);
      if (!ret)
        {
          g_warning (
            "could not create tmp dir for log "
            "file: %s",
            err->message);
          g_error_free (err);
        }
    }
  GtkWidget * log_file_path_lbl = gtk_label_new (NULL);
  if (log_file_path)
    {
      char * str = g_strdup_printf (
        "<a href=\"file://%s\">%s</a>", log_file_path,
        log_file_path);
      gtk_label_set_markup (
        GTK_LABEL (log_file_path_lbl), str);
      g_free (str);
    }
  gtk_grid_attach (grid, log_file_path_lbl, 0, 4, 1, 1);

  /* screenshot */
  GtkWidget * screenshot_heading = gtk_label_new ("");
  gtk_label_set_markup (
    GTK_LABEL (screenshot_heading), _ ("<b>Screenshot</b>"));
  gtk_grid_attach (grid, screenshot_heading, 0, 5, 1, 1);
  char * screenshot_tmpdir = NULL;
  char * screenshot_path = NULL;
  if (MAIN_WINDOW)
    {
      const char * option_keys[] = {
        "quality",
        NULL,
      };
      const char * option_vals[] = {
        "30",
        NULL,
      };
      z_gtk_generate_screenshot_image (
        GTK_WIDGET (MAIN_WINDOW), "jpeg",
        (char **) option_keys, (char **) option_vals,
        &screenshot_tmpdir, &screenshot_path, true);
    }
  GtkWidget * img = gtk_image_new ();
  if (screenshot_path)
    {
      GError *    err = NULL;
      GdkPixbuf * pixbuf = gdk_pixbuf_new_from_file_at_scale (
        screenshot_path, 580, 360, true, &err);
      gtk_widget_set_size_request (GTK_WIDGET (img), 580, 360);
      if (pixbuf)
        {
          gtk_image_set_from_pixbuf (GTK_IMAGE (img), pixbuf);
          g_object_unref (pixbuf);
        }
      else
        {
          g_warning (
            "could not get pixbuf from %s: %s",
            screenshot_path, err->message);
        }
    }
  gtk_grid_attach (grid, img, 0, 6, 1, 1);

  /* run the dialog */
  int ret = z_gtk_dialog_run (GTK_DIALOG (dialog), true);

  if (ret != GTK_RESPONSE_OK)
    return;

  char message[6000];
  strcpy (message, _ ("Sending"));

  AutomaticReportData data;
  memset (&data, 0, sizeof (AutomaticReportData));
  data.json = json_str;
  data.screenshot_path = screenshot_path;
  data.log_file_path = log_file_path;
  data.progress_nfo = progress_info_new ();
  /*strcpy (data.progress_nfo.label_str, _ ("Sending data..."));*/
  /*strcpy (data.progress_nfo.label_done_str, _ ("Done"));*/
  /*strcpy (data.progress_nfo.error_str, _ ("Failed"));*/
  GenericProgressDialogWidget * progress_dialog =
    generic_progress_dialog_widget_new ();
  generic_progress_dialog_widget_setup (
    progress_dialog, _ ("Sending..."), data.progress_nfo,
    _ ("Sending data..."), true, false);

  /* start sending in a new thread */
  GThread * thread = g_thread_new (
    "bounce_thread", (GThreadFunc) send_data, &data);

  /* run dialog */
  gtk_window_set_transient_for (
    GTK_WINDOW (progress_dialog), GTK_WINDOW (self));
  z_gtk_dialog_run (GTK_DIALOG (progress_dialog), true);

  g_thread_join (thread);

  if (
    progress_info_get_completion_type (data.progress_nfo)
    == PROGRESS_COMPLETED_SUCCESS)
    {
      gtk_dialog_set_response_sensitive (
        GTK_DIALOG (self),
        PREVIEW_AND_SEND_AUTOMATICALLY_RESPONSE, false);

      ui_show_message_printf (
        GTK_MESSAGE_INFO, false, "%s",
        _ ("Sent successfully"));
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

  progress_info_free (data.progress_nfo);

  g_free (json_str);
}

static void
on_sourcehut_response (BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  /* create new dialog */
  GtkDialogFlags flags =
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog = gtk_message_dialog_new_with_markup (
    GTK_WINDOW (self), flags, GTK_MESSAGE_ERROR,
    GTK_BUTTONS_CLOSE, NULL);

  gtk_window_set_title (
    GTK_WINDOW (dialog), _ ("Send via Sourcehut"));

  /* set top text */
  char * atag =
    g_strdup_printf ("<a href=\"%s\">", NEW_ISSUE_URL);
  char * markup = g_strdup_printf (
    _ ("Please copy the template below in a "
       "%snew issue%s."),
    atag, "</a>");
  gtk_message_dialog_set_markup (
    GTK_MESSAGE_DIALOG (dialog), markup);
  g_free (atag);
  g_free (markup);

  /* set bottom text - template */
  char * report_template = get_report_template (self, false);
  gtk_message_dialog_format_secondary_markup (
    GTK_MESSAGE_DIALOG (dialog), "%s", report_template);
  g_free (report_template);

  /* make label selectable */
  GtkLabel * label = z_gtk_message_dialog_get_label (
    GTK_MESSAGE_DIALOG (dialog), 1);
  gtk_label_set_selectable (label, true);

  /* wrap bottom text in a scrolled window */
  z_gtk_message_dialog_wrap_message_area_in_scroll (
    GTK_MESSAGE_DIALOG (dialog), 580, 320);

  /* run the dialog */
  gtk_window_present (GTK_WINDOW (dialog));

  g_signal_connect (
    G_OBJECT (dialog), "response",
    G_CALLBACK (gtk_window_destroy), NULL);
}

static void
on_email_response (BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  char * report_template = get_report_template (self, true);
  char * email_url = g_strdup_printf (
    "mailto:%s?body=%s", NEW_ISSUE_EMAIL, report_template);
  g_free (report_template);

  gtk_show_uri (
    GTK_WINDOW (self), email_url, GDK_CURRENT_TIME);
  g_free (email_url);
}

static void
on_response (
  GtkDialog * dialog,
  gint        response_id,
  gpointer    user_data)
{
  BugReportDialogWidget * self =
    Z_BUG_REPORT_DIALOG_WIDGET (user_data);

  g_debug ("response %d", response_id);

  switch (response_id)
    {
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_DELETE_EVENT:
      gtk_window_destroy (GTK_WINDOW (dialog));

      if (self->fatal)
        {
          exit (EXIT_FAILURE);
        }
      break;
    case SOURCEHUT_RESPONSE:
      on_sourcehut_response (self);
      break;
    case EMAIL_RESPONSE:
      on_email_response (self);
      break;
    case PREVIEW_AND_SEND_AUTOMATICALLY_RESPONSE:
      on_preview_and_send_automatically_response (self);
      break;
    }
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
  BugReportDialogWidget * self = g_object_new (
    BUG_REPORT_DIALOG_WIDGET_TYPE, "icon-name", "zrythm",
    "title", fatal ? _ ("Fatal Error") : _ ("Error"), NULL);

  char * markup = g_strdup_printf (
    _ ("%sPlease help us fix this by "
       "submitting a bug report"),
    msg_prefix);
  gtk_label_set_markup (self->top_lbl, markup);

  self->log = log_get_last_n_lines (LOG, 60);
  self->log_long = log_get_last_n_lines (LOG, 160);
  self->undo_stack =
    PROJECT && UNDO_MANAGER
      ? undo_stack_get_as_string (UNDO_MANAGER->undo_stack, 12)
      : g_strdup ("<undo stack uninitialized>\n");
  self->undo_stack_long =
    PROJECT && UNDO_MANAGER
      ? undo_stack_get_as_string (UNDO_MANAGER->undo_stack, 64)
      : g_strdup ("<undo stack uninitialized>\n");
  self->backtrace = g_strdup (backtrace);
  self->system_nfo = zrythm_get_system_info ();
  self->fatal = fatal;

  gtk_label_set_text (self->backtrace_lbl, self->backtrace);
  gtk_label_set_text (self->log_lbl, self->log);
  gtk_label_set_text (self->system_info_lbl, self->system_nfo);

  return self;
}

static void
dispose (BugReportDialogWidget * self)
{
  g_free_and_null (self->log);
  g_free_and_null (self->log_long);
  g_free_and_null (self->undo_stack);
  g_free_and_null (self->undo_stack_long);
  g_free_and_null (self->backtrace);
  g_free_and_null (self->system_nfo);

  G_OBJECT_CLASS (bug_report_dialog_widget_parent_class)
    ->dispose (G_OBJECT (self));

  zrythm_app->bug_report_dialog = NULL;
}

static void
bug_report_dialog_widget_class_init (
  BugReportDialogWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
bug_report_dialog_widget_init (BugReportDialogWidget * self)
{
  GtkBox * content_area =
    GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self)));

  AdwPreferencesPage * pref_page =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  gtk_box_append (content_area, GTK_WIDGET (pref_page));

  self->top_lbl = GTK_LABEL (gtk_label_new (""));
  gtk_label_set_justify (self->top_lbl, GTK_JUSTIFY_CENTER);
  gtk_widget_add_css_class (
    GTK_WIDGET (self->top_lbl), "title-4");
  AdwPreferencesGroup * pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_add (
    pref_group, GTK_WIDGET (self->top_lbl));
  adw_preferences_page_add (pref_page, pref_group);

  pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (
    pref_group, _ ("What did you do?"));
  adw_preferences_group_set_description (
    pref_group,
    _ ("Please enter as many details as possible "
       "(such as steps to reproduce)"));
  adw_preferences_page_add (pref_page, pref_group);

  self->user_input_text_view =
    GTK_TEXT_VIEW (gtk_text_view_new ());
  gtk_widget_set_size_request (
    GTK_WIDGET (self->user_input_text_view), -1, 120);
  gtk_text_view_set_accepts_tab (
    self->user_input_text_view, false);
  gtk_text_view_set_wrap_mode (
    self->user_input_text_view, GTK_WRAP_WORD_CHAR);
  adw_preferences_group_add (
    pref_group, GTK_WIDGET (self->user_input_text_view));

  pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (
    pref_group, _ ("Error details"));
  adw_preferences_page_add (pref_page, pref_group);

  {
    AdwExpanderRow * exp_row =
      ADW_EXPANDER_ROW (adw_expander_row_new ());
    adw_preferences_row_set_title (
      ADW_PREFERENCES_ROW (exp_row), _ ("Backtrace"));
    self->backtrace_lbl = GTK_LABEL (gtk_label_new (""));
    gtk_label_set_selectable (self->backtrace_lbl, true);
    GtkListBoxRow * row =
      GTK_LIST_BOX_ROW (gtk_list_box_row_new ());
    gtk_list_box_row_set_child (
      row, GTK_WIDGET (self->backtrace_lbl));
    adw_expander_row_add_row (exp_row, GTK_WIDGET (row));
    adw_preferences_group_add (
      pref_group, GTK_WIDGET (exp_row));
  }

  {
    AdwExpanderRow * exp_row =
      ADW_EXPANDER_ROW (adw_expander_row_new ());
    adw_preferences_row_set_title (
      ADW_PREFERENCES_ROW (exp_row), _ ("Log"));
    self->log_lbl = GTK_LABEL (gtk_label_new (""));
    gtk_label_set_selectable (self->log_lbl, true);
    GtkListBoxRow * row =
      GTK_LIST_BOX_ROW (gtk_list_box_row_new ());
    gtk_list_box_row_set_child (
      row, GTK_WIDGET (self->log_lbl));
    adw_expander_row_add_row (exp_row, GTK_WIDGET (row));
    adw_preferences_group_add (
      pref_group, GTK_WIDGET (exp_row));
  }

  {
    AdwExpanderRow * exp_row =
      ADW_EXPANDER_ROW (adw_expander_row_new ());
    adw_preferences_row_set_title (
      ADW_PREFERENCES_ROW (exp_row), _ ("System info"));
    self->system_info_lbl = GTK_LABEL (gtk_label_new (""));
    gtk_label_set_selectable (self->system_info_lbl, true);
    GtkListBoxRow * row =
      GTK_LIST_BOX_ROW (gtk_list_box_row_new ());
    gtk_list_box_row_set_child (
      row, GTK_WIDGET (self->system_info_lbl));
    adw_expander_row_add_row (exp_row, GTK_WIDGET (row));
    adw_preferences_group_add (
      pref_group, GTK_WIDGET (exp_row));
  }

  /* --- responses --- */

  gtk_dialog_add_buttons (
    GTK_DIALOG (self), _ ("_Close"), GTK_RESPONSE_CLOSE,
    _ ("Submit on _SourceHut"), SOURCEHUT_RESPONSE,
    _ ("Send via _email (publicly visible)"), EMAIL_RESPONSE,
    _ ("Preview and send _automatically (privately)"),
    PREVIEW_AND_SEND_AUTOMATICALLY_RESPONSE, NULL);

  g_signal_connect (
    G_OBJECT (self), "response", G_CALLBACK (on_response),
    self);
}
