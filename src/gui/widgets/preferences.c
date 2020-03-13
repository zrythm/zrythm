/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <locale.h>

#include "audio/engine.h"
#include "gui/widgets/midi_controller_mb.h"
#include "gui/widgets/preferences.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/localization.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (PreferencesWidget,
               preferences_widget,
               GTK_TYPE_DIALOG)

#define SETUP_AUDIO_BACKENDS(self) \
  ui_setup_audio_backends_combo_box (\
    self->audio_backend);

enum
{
  VALUE_COL,
  TEXT_COL,
  ID_COL,
};

static void
setup_audio_devices (
  PreferencesWidget * self)
{
  AudioBackend prev_backend =
    g_settings_get_enum (
      S_PREFERENCES, "audio-backend");
  g_settings_set_enum (
    S_PREFERENCES, "audio-backend",
    atoi (
      gtk_combo_box_get_active_id (
        self->audio_backend)));

  AudioBackend backend =
    (AudioBackend)
    g_settings_get_enum (
      S_PREFERENCES, "audio-backend");
  gtk_widget_set_visible (
    GTK_WIDGET (self->audio_backend_opts_box),
    backend == AUDIO_BACKEND_SDL ||
      backend == AUDIO_BACKEND_RTAUDIO);

#if defined(HAVE_SDL) || defined(HAVE_RTAUDIO)
  if (gtk_widget_get_visible (
        GTK_WIDGET (self->audio_backend_opts_box)))
    {
      ui_setup_device_name_combo_box (
        self->device_name_cb);
      ui_setup_buffer_size_combo_box (
        self->buffer_size_cb);
      ui_setup_samplerate_combo_box (
        self->samplerate_cb);
    }
#endif

  g_settings_set_enum (
    S_PREFERENCES, "audio-backend", prev_backend);
}

static void
on_audio_backend_changed (
  GtkComboBox *widget,
  PreferencesWidget * self)
{
  setup_audio_devices (self);
}

static void
on_language_changed (
  GtkComboBox *widget,
  PreferencesWidget * self)
{
  LocalizationLanguage lang =
    gtk_combo_box_get_active (widget);

  /* english is the default */
  if (lang == LL_EN)
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->locale_not_available),
        F_NOT_VISIBLE);
      return;
    }

  /* if locale exists */
  char * code = localization_locale_exists (lang);
  char * match = setlocale (LC_ALL, code);

  if (match)
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->locale_not_available),
        F_NOT_VISIBLE);
    }
  /* locale doesn't exist */
  else
    {
      /* show warning */
      char * str =
        ui_get_locale_not_available_string (lang);
      gtk_label_set_markup (
        self->locale_not_available, str);
      g_free (str);

      gtk_widget_set_visible (
        GTK_WIDGET (self->locale_not_available),
        F_VISIBLE);
    }
}

static void
setup_autosave_spinbutton (
  PreferencesWidget * self)
{
  GtkAdjustment * adj =
    gtk_adjustment_new (
      g_settings_get_uint (
        S_PREFERENCES, "autosave-interval"),
      0, 20, 1.0, 5.0, 0.0);
  gtk_spin_button_set_adjustment (
    self->autosave_spin,
    adj);
}

static void
setup_plugins (PreferencesWidget * self)
{
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (self->open_plugin_uis),
    g_settings_get_boolean (
      S_PREFERENCES,
      "open-plugin-uis-on-instantiate"));
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (self->keep_plugin_uis_on_top),
    g_settings_get_boolean (
      S_PREFERENCES,
      "plugin-uis-stay-on-top"));
}

static void
on_cancel_clicked (GtkWidget * widget,
          gpointer    user_data)
{
  PreferencesWidget * self =
    Z_PREFERENCES_WIDGET (user_data);

  /* TODO confirmation */

  gtk_window_close (GTK_WINDOW (self));
}

static void
on_ok_clicked (GtkWidget * widget,
                gpointer    user_data)
{
  PreferencesWidget * self =
    Z_PREFERENCES_WIDGET (user_data);

  g_settings_set_enum (
    S_PREFERENCES,
    "audio-backend",
    atoi (
      gtk_combo_box_get_active_id (
        self->audio_backend)));
  g_settings_set_enum (
    S_PREFERENCES,
    "midi-backend",
    atoi (
      gtk_combo_box_get_active_id (
        self->midi_backend)));
  g_settings_set_enum (
    S_PREFERENCES,
    "language",
    gtk_combo_box_get_active (self->language));
  g_settings_set_enum (
    S_PREFERENCES,
    "pan-algo",
    gtk_combo_box_get_active (self->pan_algo));
  g_settings_set_enum (
    S_PREFERENCES,
    "pan-law",
    gtk_combo_box_get_active (self->pan_law));
  g_settings_set_boolean (
    S_PREFERENCES,
    "open-plugin-uis-on-instantiate",
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (self->open_plugin_uis)));
  g_settings_set_boolean (
    S_PREFERENCES,
    "plugin-uis-stay-on-top",
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (
        self->keep_plugin_uis_on_top)));
  g_settings_set_uint (
    S_PREFERENCES,
    "autosave-interval",
    (unsigned int)
      gtk_spin_button_get_value (
        self->autosave_spin));
  midi_controller_mb_widget_save_settings (
    self->midi_controllers);
#ifdef _WOE32
  ui_update_vst_paths_from_entry (
    self->vst_paths_entry);
#endif

#if defined(HAVE_SDL) || defined(HAVE_RTAUDIO)
  AudioBackend backend =
    (AudioBackend)
    g_settings_get_enum (
      S_PREFERENCES, "audio-backend");
  if (backend == AUDIO_BACKEND_SDL ||
      backend == AUDIO_BACKEND_RTAUDIO)
    {
      g_settings_set_enum (
        S_PREFERENCES, "samplerate",
        gtk_combo_box_get_active (
          self->samplerate_cb));
      g_settings_set_enum (
        S_PREFERENCES, "buffer-size",
        gtk_combo_box_get_active (
          self->buffer_size_cb));
      if (backend == AUDIO_BACKEND_SDL)
        {
          g_settings_set_string (
            S_PREFERENCES, "sdl-audio-device-name",
            gtk_combo_box_text_get_active_text (
              self->device_name_cb));
        }
      else if (backend == AUDIO_BACKEND_RTAUDIO)
        {
          g_settings_set_string (
            S_PREFERENCES, "rtaudio-audio-device-name",
            gtk_combo_box_text_get_active_text (
              self->device_name_cb));
        }
    }
#endif

  /* set path */
  GFile * file =
    gtk_file_chooser_get_file (
      GTK_FILE_CHOOSER (self->zpath_fc));
  char * str =
    g_file_get_path (file);
  g_settings_set_string (
    S_GENERAL, "dir", str);
  g_free (str);

  gtk_window_close (GTK_WINDOW (self));
}

/**
 * Sets up the preferences widget.
 */
PreferencesWidget *
preferences_widget_new ()
{
  PreferencesWidget * self =
    g_object_new (PREFERENCES_WIDGET_TYPE,
                  NULL);

  SETUP_AUDIO_BACKENDS (self);
  ui_setup_midi_backends_combo_box (
    self->midi_backend);
  ui_setup_language_combo_box (self->language);
  ui_setup_pan_algo_combo_box (
    self->pan_algo);
  ui_setup_pan_law_combo_box (
    self->pan_law);
  setup_plugins (self);
  midi_controller_mb_widget_setup (
    self->midi_controllers);
  setup_autosave_spinbutton (self);

  setup_audio_devices (self);

#ifdef _WOE32
  /* setup vst_paths */
  ui_setup_vst_paths_entry (
    self->vst_paths_entry);
#endif

  char * dir = zrythm_get_dir (ZRYTHM);
  gtk_file_chooser_set_current_folder (
    GTK_FILE_CHOOSER (self->zpath_fc),
    dir);
  g_free (dir);

  return self;
}

static void
preferences_widget_class_init (
  PreferencesWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "preferences.ui");

#define BIND_CHILD(x) \
gtk_widget_class_bind_template_child ( \
  klass, \
  PreferencesWidget, \
  x)

  BIND_CHILD (categories);
  BIND_CHILD (ok);
  BIND_CHILD (audio_backend);
  BIND_CHILD (midi_backend);
  BIND_CHILD (midi_controllers);
  BIND_CHILD (language);
  BIND_CHILD (locale_not_available);
  BIND_CHILD (pan_algo);
  BIND_CHILD (pan_law);
  BIND_CHILD (open_plugin_uis);
  BIND_CHILD (keep_plugin_uis_on_top);
  BIND_CHILD (zpath_fc);
  BIND_CHILD (autosave_spin);
  BIND_CHILD (audio_backend_opts_box);
  BIND_CHILD (buffer_size_cb);
  BIND_CHILD (samplerate_cb);
  BIND_CHILD (device_name_cb);
#ifdef _WOE32
  BIND_CHILD (vst_paths_label);
  BIND_CHILD (vst_paths_entry);
#endif

  gtk_widget_class_bind_template_callback (
    klass,
    on_ok_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_cancel_clicked);

#undef BIND_CHILD
}

static void
preferences_widget_init (
  PreferencesWidget * self)
{
  g_type_ensure (MIDI_CONTROLLER_MB_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

#ifdef _WOE32
  gtk_widget_set_visible (
    GTK_WIDGET (self->vst_paths_label), 1);
  gtk_widget_set_visible (
    GTK_WIDGET (self->vst_paths_entry), 1);
#endif

  g_signal_connect (
    G_OBJECT (self->language), "changed",
    G_CALLBACK (on_language_changed), self);
  g_signal_connect (
    G_OBJECT (self->audio_backend), "changed",
    G_CALLBACK (on_audio_backend_changed), self);
}
