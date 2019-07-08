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

/**
 * \file
 *
 * Preferences window.
 */

#include "audio/engine.h"
#include "gui/widgets/midi_controller_mb.h"
#include "gui/widgets/preferences.h"
#include "settings/settings.h"
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
  TEXT_COL
};


static void
setup_plugins (PreferencesWidget * self)
{
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (self->open_plugin_uis),
    g_settings_get_int (
      S_PREFERENCES,
      "open-plugin-uis-on-instantiate"));
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
    gtk_combo_box_get_active (self->audio_backend));
  g_settings_set_enum (
    S_PREFERENCES,
    "midi-backend",
    gtk_combo_box_get_active (self->midi_backend));
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
  g_settings_set_int (
    S_PREFERENCES,
    "open-plugin-uis-on-instantiate",
  gtk_toggle_button_get_active (
    GTK_TOGGLE_BUTTON (self->open_plugin_uis)));
  midi_controller_mb_widget_save_settings (
    self->midi_controllers);

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

  return self;
}

static void
preferences_widget_class_init (
  PreferencesWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "preferences.ui");

  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    categories);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    ok);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    audio_backend);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    midi_backend);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    midi_controllers);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    language);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    pan_algo);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    pan_law);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    open_plugin_uis);
  gtk_widget_class_bind_template_callback (
    klass,
    on_ok_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_cancel_clicked);
}

static void
preferences_widget_init (PreferencesWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
