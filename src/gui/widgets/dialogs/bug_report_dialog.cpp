// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "actions/undo_manager.h"
#include "actions/undo_stack.h"
#include "gui/widgets/dialogs/bug_report_dialog.h"
#include "gui/widgets/dialogs/generic_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "utils/networking.h"
#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <yyjson.h>

G_DEFINE_TYPE (
  BugReportDialogWidget,
  bug_report_dialog_widget,
  ADW_TYPE_ALERT_DIALOG)

#define GITLAB_RESPONSE 450
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
      AdwAlertDialog * dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (
        _ ("Need More Info"), _ ("Please enter more details")));
      adw_alert_dialog_add_responses (
        ADW_ALERT_DIALOG (dialog), "ok", _ ("_OK"), nullptr);
      adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "ok");
      adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "ok");

      adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (self));
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
  Zrythm::get_version_with_capabilities (ver_with_caps, false);
  char * report_template = g_strdup_printf (
    "# What did you do?\n"
    "%s\n\n"
    "# Version\n```\n%s```\n\n"
    "# System info\n```\n%s\n```\n\n"
    "# Backtrace\n```\n%s```\n\n"
    "# Fatal\n%s\n\n"
    "# Action stack\n```\n%s```\n\n"
    "# Log\n```\n%s```",
    steps_to_reproduce, ver_with_caps, self->system_nfo, self->backtrace,
    self->fatal ? "Yes" : "No", self->undo_stack.c_str (),

    self->log.empty ()
      ? ""
      : std::accumulate (
          std::next (self->log.begin ()), self->log.end (), self->log[0],
          [] (std::string a, const std::string &b) {
            return std::move (a) + " | " + b;
          })
          .c_str ());

  char * ret;
  if (for_uri)
    ret = g_uri_escape_string (report_template, nullptr, FALSE);
  else
    ret = g_markup_escape_text (report_template, -1);

  g_free (steps_to_reproduce);
  g_free (report_template);

  return ret;
}

static char *
get_json_string (BugReportDialogWidget * self)
{
  z_debug ("{}: generating json...", __func__);

  yyjson_mut_doc * doc = yyjson_mut_doc_new (nullptr);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  z_return_val_if_fail (root, nullptr);
  yyjson_mut_doc_set_root (doc, root);

  char * steps_to_reproduce = z_gtk_text_buffer_get_full_text (
    gtk_text_view_get_buffer (self->user_input_text_view));
  yyjson_mut_obj_add_strcpy (
    doc, root, "steps_to_reproduce", steps_to_reproduce);
  g_free (steps_to_reproduce);

  yyjson_mut_obj_add_strcpy (doc, root, "extra_info", self->system_nfo);

  yyjson_mut_obj_add_strcpy (
    doc, root, "action_stack", self->undo_stack_long.c_str ());

  yyjson_mut_obj_add_strcpy (doc, root, "backtrace", self->backtrace);

  yyjson_mut_obj_add_int (doc, root, "fatal", self->fatal);

  char ver_with_caps[2000];
  Zrythm::get_version_with_capabilities (ver_with_caps, false);
  yyjson_mut_obj_add_strcpy (doc, root, "version", ver_with_caps);

  char * str = yyjson_mut_write (doc, YYJSON_WRITE_NOFLAG, nullptr);

  yyjson_mut_doc_free (doc);

  return str;
}

typedef struct AutomaticReportData
{
  ProgressInfo *          progress_nfo;
  BugReportDialogWidget * self;
  GThread *               thread;
} AutomaticReportData;

static AutomaticReportData *
automatic_report_data_new (BugReportDialogWidget * self)
{
  AutomaticReportData * data = object_new (AutomaticReportData);
  data->progress_nfo = new ProgressInfo ();
  data->self = self;
  return data;
}

static void
automatic_report_data_free (AutomaticReportData * data)
{
  object_delete_and_null (data->progress_nfo);
  object_zero_and_free (data);
}

static void *
send_data (AutomaticReportData * data)
{
  BugReportDialogWidget * self = data->self;
  try
    {
      networking::URL url (BUG_REPORT_API_ENDPOINT);
      url.post_json_no_auth (
        self->json_str, 7000,
        /* screenshot */
        { networking::URL::MultiPartMimeObject (
            "screenshot", self->screenshot_path, "image/jpeg"),
          /* log */
          networking::URL::MultiPartMimeObject (
            "log_file", self->log_file_path, "application/zstd") });
    }
  catch (const ZrythmException &e)
    {
      data->progress_nfo->mark_completed (
        ProgressInfo::CompletionType::HAS_ERROR, e.what ());
      return nullptr;
    }

  data->progress_nfo->mark_completed (
    ProgressInfo::CompletionType::SUCCESS, _ ("Sent successfully"));

  return nullptr;
}

static void
send_data_ready_cb (
  GObject *             source_object,
  GAsyncResult *        res,
  AutomaticReportData * data)
{
  adw_alert_dialog_choose_finish (ADW_ALERT_DIALOG (source_object), res);

  g_thread_join (data->thread);

  BugReportDialogWidget * self = data->self;

  if (
    data->progress_nfo->get_completion_type ()
    == ProgressInfo::CompletionType::SUCCESS)
    {
      adw_alert_dialog_set_response_enabled (
        ADW_ALERT_DIALOG (self), "automatic", false);
    }

  if (self->log_file_path)
    {
      io_remove (self->log_file_path);
      g_free_and_null (self->log_file_path);
    }
  if (self->log_file_tmpdir)
    {
      io_rmdir (self->log_file_tmpdir, Z_F_NO_FORCE);
      g_free_and_null (self->log_file_tmpdir);
    }
  if (self->screenshot_path)
    {
      io_remove (self->screenshot_path);
      g_free_and_null (self->screenshot_path);
    }
  if (self->screenshot_tmpdir)
    {
      io_rmdir (self->screenshot_tmpdir, Z_F_NO_FORCE);
      g_free_and_null (self->screenshot_tmpdir);
    }

  automatic_report_data_free (data);

  g_free_and_null (self->json_str);

  adw_dialog_force_close (ADW_DIALOG (self));
}

static void
on_send_automatically_response (
  AdwAlertDialog *        dialog,
  char *                  response,
  BugReportDialogWidget * self)
{
  if (!string_is_equal (response, "ok"))
    return;

  char message[6000];
  strcpy (message, _ ("Sending"));

  AutomaticReportData * data = automatic_report_data_new (self);
  /*strcpy (data.progress_nfo.label_str, _ ("Sending data..."));*/
  /*strcpy (data.progress_nfo.label_done_str, _ ("Done"));*/
  /*strcpy (data.progress_nfo.error_str, _ ("Failed"));*/
  GenericProgressDialogWidget * progress_dialog =
    generic_progress_dialog_widget_new ();
  generic_progress_dialog_widget_setup (
    progress_dialog, _ ("Sending..."), data->progress_nfo,
    _ ("Sending data..."), true, std::nullopt, false);

  /* start sending in a new thread */
  data->thread =
    g_thread_new ("send_data_thread", (GThreadFunc) send_data, data);

  /* run dialog */
  adw_alert_dialog_choose (
    ADW_ALERT_DIALOG (progress_dialog), GTK_WIDGET (self), nullptr,
    (GAsyncReadyCallback) send_data_ready_cb, data);
}

static void
on_preview_and_send_automatically_response (BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  /* create new dialog */
  AdwAlertDialog * dialog =
    ADW_ALERT_DIALOG (adw_alert_dialog_new (_ ("Send Automatically"), nullptr));
  adw_alert_dialog_add_responses (
    ADW_ALERT_DIALOG (dialog), "cancel", _ ("_Cancel"), "ok", _ ("_OK"),
    nullptr);
  adw_alert_dialog_set_response_appearance (
    dialog, "ok", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (dialog, "ok");

  GtkGrid * grid = GTK_GRID (gtk_grid_new ());
  z_gtk_widget_set_margin (GTK_WIDGET (grid), 4);
  gtk_grid_set_row_spacing (grid, 2);
  adw_alert_dialog_set_extra_child (dialog, GTK_WIDGET (grid));

  /* set top text */
  char * atag = g_strdup_printf ("<a href=\"%s\">", PRIVACY_POLICY_URL);
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
  gtk_label_set_markup (GTK_LABEL (json_heading), _ ("<b>Data</b>"));
  gtk_grid_attach (grid, json_heading, 0, 1, 1, 1);
  self->json_str = get_json_string (self);
  GtkWidget * json_label = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (json_label), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (json_label), false);
  GtkTextBuffer * json_label_buf =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (json_label));
#if 0
  GtkSourceLanguageManager * manager = z_gtk_source_language_manager_get ();
  GtkSourceLanguage *        lang =
    gtk_source_language_manager_get_language (manager, "json");
  gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (json_label_buf), lang);
#endif
  gtk_text_buffer_set_text (json_label_buf, self->json_str, -1);
#if 0
  GtkSourceStyleSchemeManager * style_mgr =
    gtk_source_style_scheme_manager_get_default ();
  gtk_source_style_scheme_manager_prepend_search_path (
    style_mgr, CONFIGURE_SOURCEVIEW_STYLES_DIR);
  gtk_source_style_scheme_manager_force_rescan (style_mgr);
  GtkSourceStyleScheme * scheme = gtk_source_style_scheme_manager_get_scheme (
    style_mgr, "monokai-extended-zrythm");
  gtk_source_buffer_set_style_scheme (
    GTK_SOURCE_BUFFER (json_label_buf), scheme);
#endif
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
  gtk_label_set_markup (GTK_LABEL (log_file_heading), _ ("<b>Log file</b>"));
  gtk_grid_attach (grid, log_file_heading, 0, 3, 1, 1);
  self->log_file_tmpdir = NULL;
  self->log_file_path = NULL;
/* TODO: log */
#if 0
  if (LOG && LOG->log_filepath)
    {
      /* create a zstd-compressed log file */
      GError * err = NULL;
      bool     ret = log_generate_compressed_file (
        LOG, &self->log_file_tmpdir, &self->log_file_path, &err);
      if (!ret)
        {
          z_warning(
            "could not create tmp dir for log "
            "file: %s",
            err->message);
          g_error_free (err);
        }
    }
#endif
  GtkWidget * log_file_path_lbl = gtk_label_new (nullptr);
  if (self->log_file_path)
    {
      char * str = g_strdup_printf (
        "<a href=\"file://%s\">%s</a>", self->log_file_path,
        self->log_file_path);
      gtk_label_set_markup (GTK_LABEL (log_file_path_lbl), str);
      g_free (str);
    }
  gtk_grid_attach (grid, log_file_path_lbl, 0, 4, 1, 1);

  /* screenshot */
  GtkWidget * screenshot_heading = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (screenshot_heading), _ ("<b>Screenshot</b>"));
  gtk_grid_attach (grid, screenshot_heading, 0, 5, 1, 1);
  GtkWidget * img = gtk_image_new ();
  if (self->screenshot_path)
    {
      GError *    err = NULL;
      GdkPixbuf * pixbuf = gdk_pixbuf_new_from_file_at_scale (
        self->screenshot_path, 580, 360, true, &err);
      gtk_widget_set_size_request (GTK_WIDGET (img), 580, 360);
      if (pixbuf)
        {
          GdkTexture * texture = gdk_texture_new_for_pixbuf (pixbuf);
          gtk_image_set_from_paintable (
            GTK_IMAGE (img), GDK_PAINTABLE (texture));
          g_object_unref (pixbuf);
          g_object_unref (texture);
        }
      else
        {
          z_warning (
            "could not get pixbuf from %s: %s", self->screenshot_path,
            err->message);
        }
    }
  gtk_grid_attach (grid, img, 0, 6, 1, 1);

  /* run the dialog */
  g_signal_connect (
    G_OBJECT (dialog), "response", G_CALLBACK (on_send_automatically_response),
    self);
  adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (self));
}

static void
on_gitlab_response (BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  /* create new dialog */
  AdwAlertDialog * dialog =
    ADW_ALERT_DIALOG (adw_alert_dialog_new (_ ("Send via GitLab"), nullptr));

  adw_alert_dialog_add_responses (
    ADW_ALERT_DIALOG (dialog), "ok", _ ("_OK"), nullptr);
  adw_alert_dialog_set_response_appearance (
    ADW_ALERT_DIALOG (dialog), "ok", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "ok");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "ok");

  /* set top text */
  adw_alert_dialog_format_body_markup (
    ADW_ALERT_DIALOG (dialog),
    _ ("Please copy the template below in a <a href=\"%s\">new issue</a>."),
    NEW_ISSUE_URL);

  /* create new label for template */
  char *     report_template = get_report_template (self, false);
  GtkLabel * label = GTK_LABEL (gtk_label_new (nullptr));
  gtk_label_set_markup (label, report_template);
  g_free (report_template);
  gtk_label_set_selectable (label, true);

  /* wrap template in a scrolled window */
  GtkWidget * scrolled_window = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_min_content_width (
    GTK_SCROLLED_WINDOW (scrolled_window), 580);
  gtk_scrolled_window_set_min_content_height (
    GTK_SCROLLED_WINDOW (scrolled_window), 320);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (label));

  adw_alert_dialog_set_extra_child (dialog, scrolled_window);

  /* run the dialog */
  adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (self));
}

static void
on_email_response (BugReportDialogWidget * self)
{
  if (!validate_input (self))
    return;

  char * report_template = get_report_template (self, true);
  char * email_url = g_strdup_printf (
    /* FIXME */
    "mailto:%s?body=%s", NEW_ISSUE_URL, report_template);
  g_free (report_template);

  /* TODO port to URI launcher */
  (void) email_url;
  /*gtk_show_uri (GTK_WINDOW (self), email_url, GDK_CURRENT_TIME);*/
  g_free (email_url);
}

static void
on_response_cb (
  AdwAlertDialog *        dialog,
  char *                  response,
  BugReportDialogWidget * self)
{
  if (string_is_equal (response, "cancel"))
    {
      adw_dialog_force_close (ADW_DIALOG (dialog));

      if (self->fatal)
        {
          exit (EXIT_FAILURE);
        }
    }
  else if (string_is_equal (response, "gitlab"))
    {
      on_gitlab_response (self);
    }
  else if (string_is_equal (response, "email"))
    {
      on_email_response (self);
    }
  else if (string_is_equal (response, "automatic"))
    {
      on_preview_and_send_automatically_response (self);
    }
  else
    {
      z_return_if_reached ();
    }
}

/**
 * Creates and displays the about dialog.
 */
BugReportDialogWidget *
bug_report_dialog_new (
  GtkWidget *  parent,
  const char * msg_prefix,
  const char * backtrace,
  bool         fatal)
{
  BugReportDialogWidget * self = static_cast<
    BugReportDialogWidget *> (g_object_new (
    BUG_REPORT_DIALOG_WIDGET_TYPE, "heading",
    fatal ? _ ("Fatal Error") : _ ("Error"), nullptr));

  self->screenshot_tmpdir = NULL;
  self->screenshot_path = NULL;
  if (parent)
    {
      const char * option_keys[] = {
        "quality",
        nullptr,
      };
      const char * option_vals[] = {
        "30",
        nullptr,
      };
      z_gtk_generate_screenshot_image (
        GTK_WIDGET (parent), "jpeg", (char **) option_keys,
        (char **) option_vals, &self->screenshot_tmpdir, &self->screenshot_path,
        true);
    }

  char * markup = g_strdup_printf (
    _ ("%sPlease help us fix this by "
       "submitting a bug report"),
    msg_prefix);
  gtk_label_set_markup (self->top_lbl, markup);

  self->log = Logger::getInstance ()->get_last_log_entries (60, true);
  self->log_long = Logger::getInstance ()->get_last_log_entries (160, true);
  static constexpr auto uninitialized_str = "<undo stack uninitialized>\n";
  self->undo_stack =
    PROJECT && UNDO_MANAGER && UNDO_MANAGER->undo_stack_
      ? UNDO_MANAGER->undo_stack_->get_as_string (12)
      : uninitialized_str;
  self->undo_stack_long =
    PROJECT && UNDO_MANAGER && UNDO_MANAGER->undo_stack_
      ? UNDO_MANAGER->undo_stack_->get_as_string (32)
      : uninitialized_str;
  self->backtrace = g_strdup (backtrace);
  self->system_nfo = Zrythm::get_system_info ();
  self->fatal = fatal;

  gtk_label_set_text (self->backtrace_lbl, self->backtrace);
  gtk_label_set_text (
    self->log_lbl,
    self->log.empty ()
      ? ""
      : std::accumulate (
          std::next (self->log.begin ()), self->log.end (), self->log[0],
          [] (std::string a, const std::string &b) {
            return std::move (a) + " | " + b;
          })
          .c_str ());
  gtk_label_set_text (self->system_info_lbl, self->system_nfo);

  return self;
}

static void
dispose (BugReportDialogWidget * self)
{
  std::destroy_at (&self->log);
  std::destroy_at (&self->log_long);
  std::destroy_at (&self->undo_stack);
  std::destroy_at (&self->undo_stack_long);

  g_free_and_null (self->backtrace);
  g_free_and_null (self->system_nfo);

  G_OBJECT_CLASS (bug_report_dialog_widget_parent_class)
    ->dispose (G_OBJECT (self));

  zrythm_app->bug_report_dialog = NULL;
}

static void
bug_report_dialog_widget_class_init (BugReportDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "bug_report_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, BugReportDialogWidget, x)

  BIND_CHILD (top_lbl);
  BIND_CHILD (backtrace_lbl);
  BIND_CHILD (log_lbl);
  BIND_CHILD (system_info_lbl);
  BIND_CHILD (user_input_text_view);
  gtk_widget_class_bind_template_callback (klass, on_response_cb);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
bug_report_dialog_widget_init (BugReportDialogWidget * self)
{
  std::construct_at (&self->log);
  std::construct_at (&self->log_long);
  std::construct_at (&self->undo_stack);
  std::construct_at (&self->undo_stack_long);

  gtk_widget_init_template (GTK_WIDGET (self));
}
