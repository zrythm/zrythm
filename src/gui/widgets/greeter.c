// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/engine.h"
#include "dsp/engine_alsa.h"
#include "dsp/engine_jack.h"
#include "dsp/engine_pa.h"
#include "dsp/engine_pulse.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/file_chooser_entry.h"
#include "gui/widgets/greeter.h"
#include "gui/widgets/item_factory.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/datetime.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "project/project_init_flow_manager.h"

#define FILE_NOT_FOUND_STR _ ("<File not found>")

G_DEFINE_TYPE (GreeterWidget, greeter_widget, ADW_TYPE_WINDOW)

static ProjectInfo *
project_info_new (const char * name, const char * filename)
{
  ProjectInfo * self = object_new (ProjectInfo);
  self->name = g_strdup (name);
  if (filename)
    {
      self->filename = g_strdup (filename);
      self->modified = io_file_get_last_modified_datetime (filename);
      if (self->modified == -1)
        {
          self->modified_str = g_strdup (FILE_NOT_FOUND_STR);
          self->modified = G_MAXINT64;
        }
      else
        {
          self->modified_str = datetime_epoch_to_str (self->modified, NULL);
        }
      g_return_val_if_fail (self->modified_str, NULL);
    }
  else
    {
      self->filename = g_strdup ("-");
      self->modified = 0;
      self->modified_str = g_strdup ("-");
    }

  return self;
}

static void
project_info_free (ProjectInfo * self)
{
  g_free_and_null (self->name);
  g_free_and_null (self->filename);
  g_free_and_null (self->modified_str);

  object_zero_and_free (self);
}

static void
project_info_destroy_func (void * data)
{
  project_info_free ((ProjectInfo *) data);
}

static void
project_ready_while_zrythm_running_cb (bool success, GError * error, gpointer data)
{
  if (!success)
    {
      /* just show the error and let the program continue - this was called
       * while another project is already running */
      HANDLE_ERROR_LITERAL (error, _ ("Project loading failed"));
    }
}

static void
post_finish (GreeterWidget * self, bool zrythm_already_running, bool quit)
{
  if (self)
    gtk_window_destroy (GTK_WINDOW (self));

  if (quit)
    {
      if (!zrythm_already_running)
        {
          g_application_quit (G_APPLICATION (zrythm_app));
        }
    }
  else
    {
      if (zrythm_already_running)
        {
          /* pause engine early otherwise there is a crash */
          /* TODO: this should be done inside the project init flow manager at
           * the appropriate timing */
          EngineState state;
          engine_wait_for_pause (AUDIO_ENGINE, &state, true, false);

          project_init_flow_manager_load_or_create_default_project (
            ZRYTHM->open_filename, ZRYTHM->opening_template,
            project_ready_while_zrythm_running_cb, NULL);
        }
      else
        {
          /*gtk_window_close (*/
          /*GTK_WINDOW (data->assistant));*/
          g_message ("activating zrythm_app.load_project");
          g_action_group_activate_action (
            G_ACTION_GROUP (zrythm_app), "load_project", NULL);
        }
    }
}

static void
on_project_row_activated (AdwActionRow * row, GreeterWidget * self)
{
  ProjectInfo * nfo = g_object_get_data (G_OBJECT (row), "project-info");
  g_debug ("activated %s", nfo->filename);

  ZRYTHM->open_filename = nfo->filename;
  g_return_if_fail (ZRYTHM->open_filename);
  g_message ("Loading project: %s", ZRYTHM->open_filename);
  ZRYTHM->creating_project = false;

  post_finish (self, self->zrythm_already_running, false);
}

static AdwActionRow *
create_action_row_from_project_info (GreeterWidget * self, ProjectInfo * nfo)
{
  AdwActionRow * row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), nfo->name);
  adw_action_row_set_subtitle (row, nfo->filename);

  GtkWidget * img = gtk_image_new_from_icon_name ("go-next-symbolic");
  adw_action_row_add_suffix (row, img);
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), true);
  g_signal_connect (
    G_OBJECT (row), "activated", G_CALLBACK (on_project_row_activated), self);

  g_object_set_data (G_OBJECT (row), "project-info", nfo);
  return row;
}

static void
refresh_projects (GreeterWidget * self)
{
  for (size_t i = 0; i < self->project_infos_arr->len; i++)
    {
      ProjectInfo * nfo =
        (ProjectInfo *) g_ptr_array_index (self->project_infos_arr, i);
      AdwActionRow * row = create_action_row_from_project_info (self, nfo);
      adw_preferences_group_add (
        self->recent_projects_pref_group, GTK_WIDGET (row));
    }
}

static void
refresh_templates (GreeterWidget * self)
{
  GListModel * templates = adw_combo_row_get_model (self->templates_combo_row);
  for (size_t i = 0; i < self->templates_arr->len; i++)
    {
      ProjectInfo * nfo =
        (ProjectInfo *) g_ptr_array_index (self->templates_arr, i);
      gtk_string_list_append (GTK_STRING_LIST (templates), nfo->name);
      g_object_set_data (
        G_OBJECT (g_list_model_get_item (templates, i)), "project-info", nfo);
    }
}

static gboolean
greeter_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  GreeterWidget * self)
{
  zix_sem_wait (&self->progress_status_lock);
  adw_status_page_set_title (self->status_page, self->title);
  /*gtk_label_set_text (self->status_title, self->title);*/
  /*gtk_label_set_text (self->status_description, self->description);*/
  gtk_progress_bar_set_text (self->progress_bar, self->description);
  gtk_progress_bar_set_fraction (self->progress_bar, self->progress);
  zix_sem_post (&self->progress_status_lock);

  return G_SOURCE_CONTINUE;
}

static void
on_language_changed (GObject * gobject, GParamSpec * pspec, GreeterWidget * self)
{
  AdwComboRow * combo_row = self->language_dropdown;

  LocalizationLanguage lang =
    (LocalizationLanguage) adw_combo_row_get_selected (combo_row);

  g_message ("language changed to %s", localization_get_localized_name (lang));

  /* update settings */
  g_settings_set_enum (S_P_UI_GENERAL, "language", (int) lang);

  /* if locale exists */
  if (localization_init (false, true, false))
    {
      /* enable OK */
      gtk_widget_set_sensitive (GTK_WIDGET (self->config_ok_btn), true);
      gtk_widget_set_visible (GTK_WIDGET (self->lang_error_txt), false);
    }
  /* locale doesn't exist */
  else
    {
      /* disable OK */
      gtk_widget_set_sensitive (GTK_WIDGET (self->config_ok_btn), false);

      /* show warning */
      char * str = ui_get_locale_not_available_string (lang);
      gtk_label_set_markup (self->lang_error_txt, str);
      g_free (str);

      gtk_widget_set_visible (GTK_WIDGET (self->lang_error_txt), true);
    }
}

static void
on_file_set (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  IdeFileChooserEntry * fc_entry = IDE_FILE_CHOOSER_ENTRY (gobject);

  GFile * file = ide_file_chooser_entry_get_file (fc_entry);
  char *  str = g_file_get_path (file);
  g_settings_set_string (S_P_GENERAL_PATHS, "zrythm-dir", str);
  char * str2 = g_build_filename (str, ZRYTHM_PROJECTS_DIR, NULL);
  g_settings_set_string (S_GENERAL, "last-project-dir", str);
  g_free (str);
  g_free (str2);
  g_object_unref (file);
}

static void
on_continue_to_config_clicked (GtkButton * btn, GreeterWidget * self)
{
  adw_navigation_view_push (self->nav_view, self->nav_config_page);
}

static void
begin_init (GreeterWidget * self)
{
  zrythm_app->init_finished = false;

  /* start initialization in another thread */
  self->init_thread = g_thread_new (
    "init_thread", (GThreadFunc) zrythm_app_init_thread, zrythm_app);

  /* set a source func in the main GTK thread to check when scanning finished */
  g_idle_add ((GSourceFunc) zrythm_app_prompt_for_project_func, zrythm_app);
}

static void
on_config_ok_btn_clicked (GtkButton * btn, GreeterWidget * self)
{
  gtk_window_set_title (GTK_WINDOW (self), _ ("Zrythm"));
  gtk_stack_set_visible_child_name (self->stack, "progress");

  g_settings_set_boolean (S_GENERAL, "first-run", false);

  if (g_settings_get_boolean (S_GENERAL, "first-run") != false)
    {
      ui_show_error_message (
        false,
        "Could not set 'first-run' to 'false'. "
        "There is likely a problem with your GSettings "
        "backend.");
    }

  begin_init (self);
}

static void
on_config_reset_clicked (GtkButton * btn, GreeterWidget * self)
{
  char *  dir = zrythm_get_default_user_dir ();
  GFile * gf_dir = g_file_new_for_path (dir);
  g_message ("reset to %s", dir);
  ide_file_chooser_entry_set_file (self->fc_entry, gf_dir);
  g_settings_set_string (S_P_GENERAL_PATHS, "zrythm-dir", dir);
  g_free (dir);
  g_object_unref (gf_dir);

  /* reset language */
  ui_setup_language_combo_row (self->language_dropdown);
  adw_combo_row_set_selected (self->language_dropdown, LL_EN);
  gtk_widget_set_visible (GTK_WIDGET (self->lang_error_txt), false);
}

/**
 * Sets the current status and progress percentage.
 */
void
greeter_widget_set_progress_and_status (
  GreeterWidget * self,
  const char *    title,
  const char *    description,
  const double    perc)
{
  zix_sem_wait (&self->progress_status_lock);
  if (title && !description)
    {
      g_message ("[%s]", title);
    }
  else if (!title && description)
    {
      g_message ("%s", description);
    }
  else if (title && description)
    {
      g_message ("[%s] %s", title, description);
    }
  else
    {
      g_critical ("need title and description");
    }

  if (title)
    {
      strcpy (self->title, title);
    }
  if (description)
    {
      strcpy (self->description, description);
    }
  self->progress = perc;
  zix_sem_post (&self->progress_status_lock);
}

static bool
on_close_request (GtkWindow * window, GreeterWidget * self)
{
  g_debug ("close request");

  const char * cur_page = gtk_stack_get_visible_child_name (self->stack);
  if (string_is_equal (cur_page, "first-run"))
    {
      exit (EXIT_SUCCESS);
    }

  /* close normally */
  return false;
}

static void
open_ready_cb (GtkFileDialog * dialog, GAsyncResult * res, GreeterWidget * self)
{
  GError * err = NULL;
  GFile *  file = gtk_file_dialog_open_finish (dialog, res, &err);
  if (!file)
    {
      g_message ("no project selected: %s", err->message);
      g_error_free (err);
      return;
    }

  char * path = g_file_get_path (file);
  g_return_if_fail (path);
  g_object_unref (file);

  ZRYTHM->open_filename = path;
  g_message ("Loading project: %s", ZRYTHM->open_filename);

  post_finish (self, self->zrythm_already_running, false);
}

static void
on_create_project_confirm_clicked (GtkButton * btn, GreeterWidget * self)
{
  /* get the zrythm project name */
  char * str = g_settings_get_string (S_GENERAL, "last-project-dir");
  ZRYTHM->create_project_path = g_build_filename (
    str, gtk_editable_get_text (GTK_EDITABLE (self->project_title_row)), NULL);
  g_free (str);
  g_message ("creating project at: %s", ZRYTHM->create_project_path);

  GObject * template =
    adw_combo_row_get_selected_item (self->templates_combo_row);
  ProjectInfo * selected_template = g_object_get_data (template, "project-info");
  g_return_if_fail (selected_template);

  ZRYTHM->creating_project = true;

  /* if we are loading a blank template */
  if (selected_template->filename[0] == '-')
    {
      ZRYTHM->open_filename = NULL;
      g_message ("Creating blank project");
    }
  else
    {
      ZRYTHM->open_filename = selected_template->filename;
      g_message ("Creating project from template: %s", ZRYTHM->open_filename);
      ZRYTHM->opening_template = true;
    }

  post_finish (self, self->zrythm_already_running, false);
}

static void
on_project_name_changed (GtkEditable * editable, GreeterWidget * self)
{
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->create_project_confirm_btn),
    strlen (gtk_editable_get_text (GTK_EDITABLE (self->project_title_row))) > 0);
}

static void
on_create_new_project_clicked (GtkButton * btn, GreeterWidget * self)
{
  adw_navigation_view_push (
    self->open_prj_navigation_view, self->create_project_nav_page);

  /* set project parent dir */
  char *  str = g_settings_get_string (S_GENERAL, "last-project-dir");
  GFile * str_gf = g_file_new_for_path (str);
  ide_file_chooser_entry_set_file (self->project_parent_dir_fc, str_gf);
  g_object_unref (str_gf);

  /* get next available "Untitled Project" */
  char * untitled_project = g_strdup (_ ("Untitled Project"));
  char * tmp = g_build_filename (str, untitled_project, NULL);
  char * dir = io_get_next_available_filepath (tmp);
  g_free (tmp);
  g_free (untitled_project);
  untitled_project = g_path_get_basename (dir);
  g_free (dir);
  g_free (str);
  gtk_editable_set_text (
    GTK_EDITABLE (self->project_title_row), untitled_project);
  g_free (untitled_project);

  adw_combo_row_set_selected (self->templates_combo_row, 0);
}

static void
on_open_from_path_clicked (GtkButton * btn, GreeterWidget * self)
{
  ZRYTHM->creating_project = true;

  GtkFileDialog * dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _ ("Select Project"));
  gtk_file_dialog_set_modal (dialog, true);
  GtkFileFilter * filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "application/x-zrythm-project");
  gtk_file_filter_add_suffix (filter, "zpj");
  GListStore * list_store = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (list_store, filter);
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (list_store));
  g_object_unref (list_store);
  gtk_file_dialog_open (
    dialog, GTK_WINDOW (self), NULL, (GAsyncReadyCallback) open_ready_cb, self);
}

static void
on_project_parent_dir_set (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  IdeFileChooserEntry * fc_entry = IDE_FILE_CHOOSER_ENTRY (gobject);

  GFile * file = ide_file_chooser_entry_get_file (fc_entry);
  char *  str = g_file_get_path (file);
  g_settings_set_string (S_GENERAL, "last-project-dir", str);
  g_free (str);
  g_object_unref (file);
}

void
greeter_widget_select_project (
  GreeterWidget * self,
  bool            zrythm_already_running,
  bool            is_for_new_project,
  const char *    template_to_use)
{
  gtk_window_set_title (GTK_WINDOW (self), _ ("Open a Project"));
  gtk_stack_set_visible_child_name (self->stack, "project-selector");

  /* fill recent projects */
  for (int i = 0; i < ZRYTHM->num_recent_projects; i++)
    {
      char * recent_dir = io_get_dir (ZRYTHM->recent_projects[i]);
      char * project_name = g_path_get_basename (recent_dir);

      ProjectInfo * prj_nfo =
        project_info_new (project_name, ZRYTHM->recent_projects[i]);

      if (prj_nfo)
        {
          /* if file not found */
          if (string_is_equal (prj_nfo->modified_str, FILE_NOT_FOUND_STR))
            {
              /* remove from gsettings */
              zrythm_remove_recent_project (prj_nfo->filename);

              project_info_free (prj_nfo);
            }
          else
            {
              g_ptr_array_add (self->project_infos_arr, prj_nfo);
            }
        }

      g_free (project_name);
      g_free (recent_dir);
    }

  /* fill templates */
  {
    ProjectInfo * blank_template = project_info_new (_ ("Blank project"), NULL);
    g_ptr_array_add (self->templates_arr, blank_template);
    int count = 0;
    char * template;
    while ((template = ZRYTHM->templates[count]) != NULL)
      {
        char * name = g_path_get_basename (template);
        char * filename = g_build_filename (template, PROJECT_FILE, NULL);

        ProjectInfo * template_nfo = project_info_new (name, filename);
        g_ptr_array_add (self->templates_arr, template_nfo);

        g_free (name);
        g_free (filename);

        count++;
      }
  }

  refresh_projects (self);
  refresh_templates (self);

  self->zrythm_already_running = zrythm_already_running;

  self->template = g_strdup (template_to_use);

  /* this was used for forcing the demo project on first run */
#if 0
  if (template_to_use)
    {
      ZRYTHM->creating_project = true;
      ZRYTHM->open_filename = g_build_filename (template_to_use, PROJECT_FILE, NULL);
      ZRYTHM->opening_template = true;
      g_message ("Creating project from template: %s", ZRYTHM->open_filename);

      CreateProjectDialogWidget * create_prj_dialog =
        create_project_dialog_widget_new ();
      g_signal_connect (
        G_OBJECT (create_prj_dialog), "response",
        G_CALLBACK (create_project_dialog_response_cb),
        GINT_TO_POINTER (self->zrythm_already_running));
      gtk_window_set_transient_for (
        GTK_WINDOW (create_prj_dialog), GTK_WINDOW (self));
      gtk_window_present (GTK_WINDOW (create_prj_dialog));
    }
#endif

  if (is_for_new_project)
    {
      on_create_new_project_clicked (self->create_new_project_btn, self);
    }
}

GreeterWidget *
greeter_widget_new (
  ZrythmApp * app,
  GtkWindow * parent,
  bool        is_startup,
  bool        is_for_new_project)
{
  GreeterWidget * self = g_object_new (
    GREETER_WIDGET_TYPE, "application", G_APPLICATION (app), "title",
    PROGRAM_NAME, "transient-for", parent, NULL);
  g_return_val_if_fail (Z_IS_GREETER_WIDGET (self), NULL);

  /* if not startup, just proceed to select a project */
  if (!is_startup)
    {
      greeter_widget_select_project (self, true, is_for_new_project, NULL);
    }
  /* else if not first run, skip to the "progress" part (don't show welcome UI) */
  else if (is_startup && !app->is_first_run)
    {
      gtk_window_set_title (GTK_WINDOW (self), _ ("Zrythm"));
      gtk_stack_set_visible_child_name (self->stack, "progress");
      begin_init (self);
    }

  return self;
}

static void
finalize (GreeterWidget * self)
{
  /* "first run" is now irrelevant - set it to false to prevent issues with
   * new/open */
  zrythm_app->is_first_run = false;

  if (self->init_thread)
    {
      g_thread_join (self->init_thread);
      self->init_thread = NULL;
    }

  g_ptr_array_unref (self->recent_projects_item_factories);
  g_ptr_array_unref (self->templates_item_factories);

  zix_sem_destroy (&self->progress_status_lock);

  G_OBJECT_CLASS (greeter_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
greeter_widget_init (GreeterWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* -- welcome part */

  gtk_widget_set_size_request (GTK_WIDGET (self->welcome_carousel), 640, -1);

  char * read_manual_txt = g_strdup_printf (
    _ ("If this is your first time using Zrythm, we suggest going through the 'Getting Started' section in the %suser manual%s."),
    "<a href=\"" USER_MANUAL_URL "\">", "</a>");
  adw_status_page_set_description (
    self->read_manual_status_page, read_manual_txt);
  g_free (read_manual_txt);

#if !defined(INSTALLER_VER) || defined(TRIAL_VER)
  char * donations = g_strdup_printf (
    _ ("Zrythm relies on donations and purchases "
       "to sustain development. If you enjoy the "
       "software, please consider %sdonating%s or "
       "%sbuying an installer%s."),
    "<a href=\"" DONATION_URL "\">", "</a>", "<a href=\"" PURCHASE_URL "\">",
    "</a>");
  adw_status_page_set_description (self->donate_status_page, donations);
  g_free (donations);
#else
  adw_carousel_remove (
    self->welcome_carousel, GTK_WIDGET (self->donate_status_page));
#endif

#ifdef FLATPAK_BUILD
  adw_status_page_set_description (
    self->about_flatpak_status_page,
    _ ("Only plugins installed via Flatpak are supported."));
#else
  adw_carousel_remove (
    self->welcome_carousel, GTK_WIDGET (self->about_flatpak_status_page));
#endif

#if 0
  char * legal_nfo = g_strdup_printf (
    _ ("Zrythm and the Zrythm logo are "
       "%strademarks of Alexandros Theodotou%s, registered in the UK and other countries."),
    "<a href=\"" TRADEMARK_POLICY_URL "\">", "</a>");
  adw_status_page_set_description (self->legal_info_status_page, legal_nfo);
  g_free (legal_nfo);
#endif

  g_signal_connect (
    G_OBJECT (self->continue_to_config_btn), "clicked",
    G_CALLBACK (on_continue_to_config_clicked), self);

  /* -- config part -- */

  self->lang_error_txt = GTK_LABEL (gtk_label_new (""));

  AdwPreferencesPage * pref_page = self->pref_page;

  /* just one preference group with everything for now */
  AdwPreferencesGroup * pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (pref_group, _ ("Initial Configuration"));
  adw_preferences_page_add (pref_page, pref_group);

  /* setup languages */
  {
    self->language_dropdown = ADW_COMBO_ROW (adw_combo_row_new ());
    ui_setup_language_combo_row (self->language_dropdown);
    AdwActionRow * row = ADW_ACTION_ROW (self->language_dropdown);
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Language"));
    adw_action_row_set_subtitle (row, _ ("Preferred language"));
    g_signal_connect (
      G_OBJECT (self->language_dropdown), "notify::selected",
      G_CALLBACK (on_language_changed), self);
    on_language_changed (NULL, NULL, self);
    adw_preferences_group_add (pref_group, GTK_WIDGET (row));
  }

  /* set zrythm dir */
  char * dir = zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  {
    AdwActionRow * row = ADW_ACTION_ROW (adw_action_row_new ());
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("User path"));
    adw_action_row_set_subtitle (row, _ ("Location to save user files"));
    self->fc_entry = IDE_FILE_CHOOSER_ENTRY (ide_file_chooser_entry_new (
      _ ("Select a directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER));
    GFile * dir_gf = g_file_new_for_path (dir);
    ide_file_chooser_entry_set_file (self->fc_entry, dir_gf);
    g_object_unref (dir_gf);
    g_signal_connect_data (
      self->fc_entry, "notify::file", G_CALLBACK (on_file_set), self, NULL,
      G_CONNECT_AFTER);
    g_settings_set_string (S_P_GENERAL_PATHS, "zrythm-dir", dir);
    gtk_widget_set_valign (GTK_WIDGET (self->fc_entry), GTK_ALIGN_CENTER);
    adw_action_row_add_suffix (row, GTK_WIDGET (self->fc_entry));
    adw_preferences_group_add (pref_group, GTK_WIDGET (row));
  }

  /* add error text */
  gtk_widget_add_css_class (GTK_WIDGET (self->lang_error_txt), "error");
  adw_preferences_group_add (pref_group, GTK_WIDGET (self->lang_error_txt));

  /* set appropriate backend */
  engine_set_default_backends (false);

  /* set the last project dir to the default one */
  char * projects_dir = g_build_filename (dir, ZRYTHM_PROJECTS_DIR, NULL);
  g_settings_set_string (S_GENERAL, "last-project-dir", projects_dir);
  g_free (dir);
  g_free (projects_dir);

  g_signal_connect (
    self->config_ok_btn, "clicked", G_CALLBACK (on_config_ok_btn_clicked), self);
  g_signal_connect (
    self->config_reset_btn, "clicked", G_CALLBACK (on_config_reset_clicked),
    self);
  g_signal_connect (self, "close-request", G_CALLBACK (on_close_request), self);

  /* -- progress part -- */

  zix_sem_init (&self->progress_status_lock, 1);
  gtk_progress_bar_set_fraction (self->progress_bar, 0.0);

  self->tick_cb_id = gtk_widget_add_tick_callback (
    (GtkWidget *) self, (GtkTickCallback) greeter_tick_cb, self, NULL);

  /* -- projects part -- */

  self->recent_projects_item_factories =
    g_ptr_array_new_with_free_func (item_factory_free_func);
  self->templates_item_factories =
    g_ptr_array_new_with_free_func (item_factory_free_func);
  self->project_infos_arr =
    g_ptr_array_new_with_free_func (project_info_destroy_func);
  self->templates_arr =
    g_ptr_array_new_with_free_func (project_info_destroy_func);

  self
    ->project_parent_dir_fc = IDE_FILE_CHOOSER_ENTRY (ide_file_chooser_entry_new (
    _ ("Select parent directory to save the project in"),
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER));
  gtk_widget_set_hexpand (GTK_WIDGET (self->project_parent_dir_fc), true);
  gtk_widget_set_valign (
    GTK_WIDGET (self->project_parent_dir_fc), GTK_ALIGN_CENTER);
  g_signal_connect_data (
    self->project_parent_dir_fc, "notify::file",
    G_CALLBACK (on_project_parent_dir_set), self, NULL, G_CONNECT_AFTER);
  adw_action_row_add_suffix (
    self->project_parent_dir_row, GTK_WIDGET (self->project_parent_dir_fc));

  g_signal_connect (
    self->project_title_row, "changed", G_CALLBACK (on_project_name_changed),
    self);

  g_signal_connect (
    self->create_new_project_btn, "clicked",
    G_CALLBACK (on_create_new_project_clicked), self);
  g_signal_connect (
    self->select_folder_btn, "clicked", G_CALLBACK (on_open_from_path_clicked),
    self);
  g_signal_connect (
    self->create_project_confirm_btn, "clicked",
    G_CALLBACK (on_create_project_confirm_clicked), self);

  /* close on escape */
  z_gtk_window_make_escapable (GTK_WINDOW (self));
}

static void
greeter_widget_class_init (GreeterWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (wklass, "greeter.ui");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_DIALOG);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (wklass, GreeterWidget, x)

  BIND_CHILD (stack);
  BIND_CHILD (welcome_carousel);
  BIND_CHILD (continue_to_config_btn);
  BIND_CHILD (read_manual_status_page);
  BIND_CHILD (donate_status_page);
  BIND_CHILD (about_flatpak_status_page);
  /*BIND_CHILD (legal_info_status_page);*/
  BIND_CHILD (nav_view);
  BIND_CHILD (nav_config_page);
  BIND_CHILD (pref_page);
  BIND_CHILD (config_reset_btn);
  BIND_CHILD (config_ok_btn);
  BIND_CHILD (status_page);
  BIND_CHILD (progress_bar);
  /*BIND_CHILD (status_title);*/
  /*BIND_CHILD (status_description);*/
  BIND_CHILD (recent_projects_pref_group);
  BIND_CHILD (create_new_project_btn);
  BIND_CHILD (select_folder_btn);
  BIND_CHILD (open_prj_navigation_view);
  BIND_CHILD (create_project_nav_page);
  BIND_CHILD (project_title_row);
  BIND_CHILD (project_parent_dir_row);
  BIND_CHILD (templates_combo_row);
  BIND_CHILD (create_project_confirm_btn);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
