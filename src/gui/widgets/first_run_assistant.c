/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2020 Ryan Gonzalez <rymg19 at gmail dot com>
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
 */

#include "audio/engine.h"
#include "audio/engine_alsa.h"
#include "audio/engine_jack.h"
#include "audio/engine_pa.h"
#include "audio/engine_pulse.h"
#include "gui/widgets/first_run_assistant.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/file_chooser_button.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Thread that scans for plugins after the first
 * run.
 */
static void *
scan_plugins_after_first_run_thread (
  gpointer data)
{
  g_message ("scanning...");

  plugin_manager_scan_plugins (
    ZRYTHM->plugin_manager,
    0.7, &ZRYTHM->progress);

  zrythm_app->init_finished = true;

  g_message ("done");

  return NULL;
}

static void
on_first_run_assistant_apply (
  GtkAssistant * assistant,
  gpointer       user_data)
{
  g_message ("on first run assistant apply...");

  g_settings_set_boolean (
    S_GENERAL, "first-run", false);

  zrythm_app->init_finished = false;

  /* start plugin scanning in another thread */
  zrythm_app->init_thread =
    g_thread_new (
      "scan_plugins_after_first_run_thread",
      (GThreadFunc)
      scan_plugins_after_first_run_thread,
      zrythm_app);

  /* set a source func in the main GTK thread to
   * check when scanning finished */
  g_idle_add (
    (GSourceFunc)
    zrythm_app_prompt_for_project_func, zrythm_app);

  gtk_window_destroy (GTK_WINDOW (assistant));

#if 0
  /* close the first run assistant if it ran
   * before */
  if (self->assistant)
    {
      DESTROY_LATER (_assistant);
      self->first_run_assistant = NULL;
    }
#endif

  g_message ("done apply");
}

static void
on_first_run_assistant_cancel (void)
{
  g_message ("%s: cancel", __func__);

  exit (0);
}

static void
on_reset_clicked (
  GtkButton *      widget,
  FileChooserButtonWidget * fc)
{
  char * dir =
    zrythm_get_default_user_dir ();
  g_message ("reset to %s", dir);
  file_chooser_button_widget_set_path (fc, dir);
  g_settings_set_string (
    S_P_GENERAL_PATHS, "zrythm-dir", dir);
  g_free (dir);
}

/**
 * Validates the current audio/MIDI backend
 * selection.
 */
static void
audio_midi_backend_selection_validate (
  GtkBuilder * builder)
{
  GtkAssistant * assistant =
    GTK_ASSISTANT (
      gtk_builder_get_object (
        builder, "assistant"));
  GtkComboBox * audio_backend =
    GTK_COMBO_BOX (
      gtk_builder_get_object (
        builder, "audio_backend"));
  GtkComboBox * midi_backend =
    GTK_COMBO_BOX (
      gtk_builder_get_object (
        builder, "midi_backend"));

  AudioBackend ab =
    (AudioBackend)
    atoi (
      gtk_combo_box_get_active_id (
        audio_backend));
  MidiBackend mb =
    (MidiBackend)
    atoi (
      gtk_combo_box_get_active_id (
        midi_backend));
  g_message (
    "selected audio backend: %s, "
    "selected MIDI backend: %s",
    audio_backend_str[ab],
    midi_backend_str[mb]);

  /* test audio backends */
  switch (ab)
    {
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      if (engine_jack_test (GTK_WINDOW (assistant)))
        return;
      break;
#endif
#ifdef HAVE_ALSA
    case AUDIO_BACKEND_ALSA:
#if 0
      if (engine_alsa_test (GTK_WINDOW (self)))
        return;
#endif
      break;
#endif
#ifdef HAVE_PULSEAUDIO
    case AUDIO_BACKEND_PULSEAUDIO:
      if (engine_pulse_test (GTK_WINDOW (assistant)))
        return;
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      if (engine_pa_test (GTK_WINDOW (assistant)))
        return;
      break;
#endif
    default:
      break;
    }

  /* test MIDI backends */
  switch (mb)
    {
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      if (engine_jack_test (GTK_WINDOW (assistant)))
        return;
      break;
#endif
#ifdef HAVE_ALSA
    case MIDI_BACKEND_ALSA:
#if 0
      if (engine_alsa_test (GTK_WINDOW (self)))
        return;
#endif
      break;
#endif
    default:
      break;
    }

  if (ab != AUDIO_BACKEND_JACK &&
      mb == MIDI_BACKEND_JACK)
    {
      ui_show_error_message (
        GTK_WINDOW (assistant), false,
        _("Backend combination not supported"));
      return;
    }

  ui_show_message_full (
    GTK_WINDOW (assistant), GTK_MESSAGE_INFO, false,
    _("The selected backends are operational"));
  return;
}

static void
on_test_backends_clicked (
  GtkButton * widget,
  GtkBuilder * builder)
{
  audio_midi_backend_selection_validate (builder);
}

static void
on_audio_backend_changed (
  GtkComboBox *  widget,
  GtkAssistant * assistant)
{
  /* update settings */
  g_settings_set_enum (
    S_P_GENERAL_ENGINE,
    "audio-backend",
    atoi (gtk_combo_box_get_active_id (widget)));
}

static void
on_midi_backend_changed (
  GtkComboBox *  widget,
  GtkAssistant * assistant)
{
  AudioBackend ab =
    g_settings_get_enum (
      S_P_GENERAL_ENGINE, "audio-backend");
  MidiBackend mb =
    (MidiBackend)
    atoi (gtk_combo_box_get_active_id (widget));
  if (ab != AUDIO_BACKEND_JACK &&
      mb == MIDI_BACKEND_JACK)
    {
      ui_show_error_message (
        GTK_WINDOW (assistant), false,
        _("JACK MIDI can only be used with JACK "
        "audio"));
      gtk_combo_box_set_active (widget, false);
      return;
    }

  /* update settings */
  g_settings_set_enum (
    S_P_GENERAL_ENGINE, "midi-backend", mb);
}

static void
on_language_changed (
  GObject    * gobject,
  GParamSpec * pspec,
  GtkBuilder * builder)
{
  GtkDropDown * dropdown =
    GTK_DROP_DOWN (
      gtk_builder_get_object (
        builder, "language_dropdown"));

  g_message ("language changed");
  LocalizationLanguage lang =
    (LocalizationLanguage)
    gtk_drop_down_get_selected (dropdown);

  /* update settings */
  g_settings_set_enum (
    S_P_UI_GENERAL, "language", (int) lang);

  GtkAssistant * assistant =
    GTK_ASSISTANT (
      gtk_builder_get_object (
        builder, "assistant"));
  GtkLabel * locale_not_available =
    GTK_LABEL (
      gtk_builder_get_object (
        builder, "locale_not_available"));

  /* if locale exists */
  if (localization_init (false, true))
    {
      /* enable "Next" */
      gtk_assistant_set_page_complete (
        assistant,
        gtk_assistant_get_nth_page (assistant, 0),
        1);
      gtk_widget_set_visible (
        GTK_WIDGET (locale_not_available), false);
    }
  /* locale doesn't exist */
  else
    {
      /* disable "Next" */
      gtk_assistant_set_page_complete (
        assistant,
        gtk_assistant_get_nth_page (assistant, 0),
        0);

      /* show warning */
      char * str =
        ui_get_locale_not_available_string (lang);
      gtk_label_set_markup (
        locale_not_available, str);
      g_free (str);

      gtk_widget_set_visible (
        GTK_WIDGET (locale_not_available), true);
    }
}

static void
on_file_set (
  GtkFileChooserNative * file_chooser_native,
  gint                   response_id,
  gpointer               user_data)
{
  GFile * file =
    gtk_file_chooser_get_file (
      GTK_FILE_CHOOSER (file_chooser_native));
  char * str =
    g_file_get_path (file);
  g_settings_set_string (
    S_P_GENERAL_PATHS, "zrythm-dir", str);
  char * str2 =
    g_build_filename (
      str, ZRYTHM_PROJECTS_DIR, NULL);
  g_settings_set_string (
    S_GENERAL, "last-project-dir", str);
  g_free (str);
  g_free (str2);
}

/**
 * Runs the first run assistant.
 */
void
first_run_assistant_widget_present (
  GtkWindow * parent)
{
  GtkBuilder * builder =
    gtk_builder_new_from_resource (
      RESOURCES_TEMPLATE_PATH
      "/first_run_assistant.ui");
  GtkAssistant * assistant =
    GTK_ASSISTANT (
      gtk_builder_get_object (
        builder, "assistant"));

  /* setup languages */
  GtkDropDown * language_dropdown =
    GTK_DROP_DOWN (
      gtk_builder_get_object (
        builder, "language_dropdown"));
  ui_setup_language_dropdown (language_dropdown);

  /* setup backends */
  GtkComboBox * audio_backend =
    GTK_COMBO_BOX (
      gtk_builder_get_object (
        builder, "audio_backend"));
  GtkComboBox * midi_backend =
    GTK_COMBO_BOX (
      gtk_builder_get_object (
        builder, "midi_backend"));
  ui_setup_audio_backends_combo_box (
    audio_backend);
  ui_setup_midi_backends_combo_box (
    midi_backend);

  /* set zrythm dir */
  GtkBox * fc_btn_box =
    GTK_BOX (
      gtk_builder_get_object (
        builder, "fc_btn_box"));
  FileChooserButtonWidget * fc_btn =
    file_chooser_button_widget_new (
      NULL,
      _("Select a directory"),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_box_append (fc_btn_box, GTK_WIDGET (fc_btn));
  char * dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  file_chooser_button_widget_set_path (
    fc_btn, dir);
  g_settings_set_string (
    S_P_GENERAL_PATHS, "zrythm-dir", dir);

  /* set the last project dir to the default one */
  char * projects_dir  =
    g_build_filename (
      dir, ZRYTHM_PROJECTS_DIR, NULL);
  g_settings_set_string (
    S_GENERAL, "last-project-dir", projects_dir);
  g_free (dir);
  g_free (projects_dir);

  /*g_message (*/
    /*"n pages %d",*/
    /*gtk_assistant_get_n_pages (*/
      /*GTK_ASSISTANT (self)));*/

  GtkButton * reset =
    GTK_BUTTON (
      gtk_builder_get_object (
        builder, "reset"));
  GtkButton * test_backends =
    GTK_BUTTON (
      gtk_builder_get_object (
        builder, "test_backends"));
#ifdef _WOE32
  gtk_widget_set_visible (
    GTK_WIDGET (test_backends), 0);
#endif

  gtk_window_set_transient_for (
    GTK_WINDOW (assistant), parent);

  g_signal_connect (
    G_OBJECT (audio_backend), "changed",
    G_CALLBACK (on_audio_backend_changed),
    assistant);
  g_signal_connect (
    G_OBJECT (midi_backend), "changed",
    G_CALLBACK (on_midi_backend_changed),
    assistant);
  g_signal_connect (
    G_OBJECT (language_dropdown), "notify::selected",
    G_CALLBACK (on_language_changed), builder);
  file_chooser_button_widget_set_response_callback (
    fc_btn, G_CALLBACK (on_file_set), builder,
    NULL);
  g_signal_connect (
    G_OBJECT (reset), "clicked",
    G_CALLBACK (on_reset_clicked), fc_btn);
  g_signal_connect (
    G_OBJECT (test_backends), "clicked",
    G_CALLBACK (on_test_backends_clicked), builder);

  g_signal_connect (
    G_OBJECT (assistant), "apply",
    G_CALLBACK (on_first_run_assistant_apply),
    zrythm_app);
  g_signal_connect (
    G_OBJECT (assistant), "cancel",
    G_CALLBACK (on_first_run_assistant_cancel),
    zrythm_app);

  gtk_window_present (GTK_WINDOW (assistant));
}
