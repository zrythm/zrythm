// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 */

#include "dsp/engine.h"
#include "dsp/engine_alsa.h"
#include "dsp/engine_jack.h"
#include "dsp/engine_pa.h"
#include "dsp/engine_pulse.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/dialogs/first_run.h"
#include "gui/widgets/file_chooser_entry.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <adwaita.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (FirstRunWindow, first_run_window, ADW_TYPE_WINDOW)

/**
 * Thread that scans for plugins after the first
 * run.
 */
static void *
scan_plugins_after_first_run_thread (gpointer data)
{
  g_message ("scanning...");

  plugin_manager_scan_plugins (ZRYTHM->plugin_manager, 0.7, &ZRYTHM->progress);

  zrythm_app->init_finished = true;

  g_message ("done");

  return NULL;
}

static void
on_ok_clicked (GtkButton * btn, FirstRunWindow * self)
{
  g_message ("first run dialog OK pressed");

  zrythm_app->is_first_run = true;

  g_settings_set_boolean (S_GENERAL, "first-run", false);

  if (g_settings_get_boolean (S_GENERAL, "first-run") != false)
    {
      ui_show_error_message (
        false,
        "Could not set 'first-run' to 'false'. "
        "There is likely a problem with your GSettings "
        "backend.");
    }

  zrythm_app->init_finished = false;

  /* start plugin scanning in another thread */
  zrythm_app->init_thread = g_thread_new (
    "scan_plugins_after_first_run_thread",
    (GThreadFunc) scan_plugins_after_first_run_thread, zrythm_app);

  /* set a source func in the main GTK thread to
   * check when scanning finished */
  g_idle_add ((GSourceFunc) zrythm_app_prompt_for_project_func, zrythm_app);

  g_message ("queued prompt for project");

  gtk_window_destroy (GTK_WINDOW (self));
}

static void
on_reset_clicked (GtkButton * btn, FirstRunWindow * self)
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

static void
on_cancel_or_close (GtkWidget * btn, FirstRunWindow * self)
{
  exit (EXIT_SUCCESS);
}

static void
on_language_changed (GObject * gobject, GParamSpec * pspec, FirstRunWindow * self)
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
      gtk_widget_set_sensitive (GTK_WIDGET (self->ok_btn), true);
      gtk_widget_set_visible (GTK_WIDGET (self->lang_error_txt), false);
    }
  /* locale doesn't exist */
  else
    {
      /* disable OK */
      gtk_widget_set_sensitive (GTK_WIDGET (self->ok_btn), false);

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

/**
 * Returns a new instance.
 */
FirstRunWindow *
first_run_window_new (GtkWindow * parent)
{
  FirstRunWindow * self = g_object_new (FIRST_RUN_WINDOW_TYPE, NULL);

  gtk_window_set_transient_for (GTK_WINDOW (self), parent);

  return self;
}

static void
dispose (FirstRunWindow * self)
{
  G_OBJECT_CLASS (first_run_window_parent_class)->dispose (G_OBJECT (self));
}

static void
first_run_window_init (FirstRunWindow * self)
{
  g_type_ensure (ADW_TYPE_TOOLBAR_VIEW);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->lang_error_txt = GTK_LABEL (gtk_label_new (""));

  AdwPreferencesPage * pref_page = self->pref_page;
  /*gtk_widget_set_vexpand (GTK_WIDGET (pref_page), true);*/

  /* just one preference group with everything for
   * now */
  AdwPreferencesGroup * pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (pref_group, _ ("Initial Configuration"));
  adw_preferences_page_add (pref_page, pref_group);

  AdwActionRow * row;

  /* setup languages */
  self->language_dropdown = ADW_COMBO_ROW (adw_combo_row_new ());
  ui_setup_language_combo_row (self->language_dropdown);
  row = ADW_ACTION_ROW (self->language_dropdown);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Language"));
  adw_action_row_set_subtitle (row, _ ("Preferred language"));
  g_signal_connect (
    G_OBJECT (self->language_dropdown), "notify::selected",
    G_CALLBACK (on_language_changed), self);
  on_language_changed (NULL, NULL, self);
  adw_preferences_group_add (pref_group, GTK_WIDGET (row));

  /* set zrythm dir */
  row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("User path"));
  adw_action_row_set_subtitle (row, _ ("Location to save user files"));
  self->fc_entry = IDE_FILE_CHOOSER_ENTRY (ide_file_chooser_entry_new (
    _ ("Select a directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER));
  char *  dir = zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
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

  gtk_window_set_default_size (GTK_WINDOW (self), 520, 296);

  g_signal_connect (self->ok_btn, "clicked", G_CALLBACK (on_ok_clicked), self);
  g_signal_connect (
    self->reset_btn, "clicked", G_CALLBACK (on_reset_clicked), self);
  g_signal_connect (
    self->cancel_btn, "clicked", G_CALLBACK (on_cancel_or_close), self);
  g_signal_connect (
    self, "close-request", G_CALLBACK (on_cancel_or_close), self);
}

static void
first_run_window_class_init (FirstRunWindowClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (wklass, "first_run_window.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (wklass, FirstRunWindow, x)

  BIND_CHILD (pref_page);
  BIND_CHILD (ok_btn);
  BIND_CHILD (reset_btn);
  BIND_CHILD (cancel_btn);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
