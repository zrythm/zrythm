/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_PREFERENCES_H__
#define __GUI_WIDGETS_PREFERENCES_H__

#include <gtk/gtk.h>

#define PREFERENCES_WIDGET_TYPE \
  (preferences_widget_get_type ())
G_DECLARE_FINAL_TYPE (PreferencesWidget,
                      preferences_widget,
                      Z,
                      PREFERENCES_WIDGET,
                      GtkDialog)

#define MW_PREFERENCES ZRYTHM->preferences

typedef struct Preferences Preferences;
typedef struct _MidiControllerMbWidget
  MidiControllerMbWidget;

typedef struct _PreferencesWidget
{
  GtkDialog                parent_instance;
  GtkNotebook *            categories;
  GtkButton *              cancel;
  GtkButton *              ok;
  GtkComboBox *            audio_backend;
  GtkComboBox *            midi_backend;
  GtkComboBox *            pan_algo;
  GtkComboBox *            pan_law;
  MidiControllerMbWidget * midi_controllers;
  Preferences *            preferences;
  GtkCheckButton *         open_plugin_uis;
  GtkCheckButton *         keep_plugin_uis_on_top;
  GtkComboBox *            language;
  GtkLabel *               locale_not_available;
  GtkSpinButton *          autosave_spin;

  /** Zrythm path chooser. */
  GtkFileChooserButton *   zpath_fc;

  GtkBox *             audio_backend_opts_box;
  GtkComboBox *        buffer_size_cb;
  GtkComboBox *        samplerate_cb;
  GtkComboBoxText *        device_name_cb;

#ifdef _WIN32
  GtkLabel *           vst_paths_label;
  GtkEntry *           vst_paths_entry;
#endif
} PreferencesWidget;

PreferencesWidget *
preferences_widget_new (void);

#endif
