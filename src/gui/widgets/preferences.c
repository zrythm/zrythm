/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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

enum
{
  VALUE_COL,
  TEXT_COL
};

static GtkTreeModel *
create_audio_backend_model (void)
{
  const int values[NUM_AUDIO_BACKENDS] = {
    AUDIO_BACKEND_NONE,
    AUDIO_BACKEND_JACK,
    AUDIO_BACKEND_ALSA,
    AUDIO_BACKEND_PORT_AUDIO,
  };
  const gchar *labels[NUM_AUDIO_BACKENDS] = {
    "- None -"
    "Jack",
    "Alsa (not implemented yet)",
    "Port Audio",
  };

  GtkTreeIter iter;
  GtkListStore *store;
  gint i;

  store = gtk_list_store_new (2,
                              G_TYPE_INT,
                              G_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          VALUE_COL, values[i],
                          TEXT_COL, labels[i],
                          -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
setup_audio (PreferencesWidget * self)
{
  GtkCellRenderer *renderer;
  gtk_combo_box_set_model (self->audio_backend,
                           create_audio_backend_model ());
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self->audio_backend));
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->audio_backend), renderer, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self->audio_backend), renderer,
    "text", TEXT_COL,
    NULL);

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (self->audio_backend),
    g_settings_get_enum (
      S_PREFERENCES,
      "audio-backend"));
}

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
    "language",
    gtk_combo_box_get_active (self->language));
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

  setup_audio (self);
  ui_setup_language_combo_box (self->language);
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
    midi_controllers);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    language);
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
