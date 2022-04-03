/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * File browser.
 */

#include "audio/engine.h"
#include "audio/sample_processor.h"
#include "gui/widgets/file_browser_filters.h"
#include "gui/widgets/volume.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FileBrowserFiltersWidget,
  file_browser_filters_widget,
  GTK_TYPE_BOX)

static void
toggles_changed (
  GtkToggleButton *          btn,
  FileBrowserFiltersWidget * self)
{
  if (gtk_toggle_button_get_active (btn))
    {
      /* block signals, unset all, unblock */
      g_signal_handlers_block_by_func (
        self->toggle_audio, toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_midi, toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_presets, toggles_changed, self);

      if (btn == self->toggle_audio)
        {
          S_SET_ENUM (
            S_UI_FILE_BROWSER, "filter",
            FILE_BROWSER_FILTER_AUDIO);
          gtk_toggle_button_set_active (
            self->toggle_midi, 0);
          gtk_toggle_button_set_active (
            self->toggle_presets, 0);
        }
      else if (btn == self->toggle_midi)
        {
          S_SET_ENUM (
            S_UI_FILE_BROWSER, "filter",
            FILE_BROWSER_FILTER_MIDI);
          gtk_toggle_button_set_active (
            self->toggle_audio, 0);
          gtk_toggle_button_set_active (
            self->toggle_presets, 0);
        }
      else if (btn == self->toggle_presets)
        {
          S_SET_ENUM (
            S_UI_FILE_BROWSER, "filter",
            FILE_BROWSER_FILTER_PRESET);
          gtk_toggle_button_set_active (
            self->toggle_midi, false);
          gtk_toggle_button_set_active (
            self->toggle_audio, false);
        }

      g_signal_handlers_unblock_by_func (
        self->toggle_audio, toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_midi, toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_presets, toggles_changed, self);
    }
  else
    {
      S_SET_ENUM (
        S_UI_FILE_BROWSER, "filter",
        FILE_BROWSER_FILTER_NONE);
    }

  self->refilter_files (self->owner);
}

/**
 * Sets up a FileBrowserFiltersWidget.
 */
void
file_browser_filters_widget_setup (
  FileBrowserFiltersWidget * self,
  GtkWidget *                owner,
  GenericCallback            refilter_files_cb)
{
  self->owner = owner;
  self->refilter_files = refilter_files_cb;
}

static void
file_browser_filters_widget_class_init (
  FileBrowserFiltersWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "file_browser_filters.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, FileBrowserFiltersWidget, x)

  BIND_CHILD (toggle_audio);
  BIND_CHILD (toggle_midi);
  BIND_CHILD (toggle_presets);

#undef BIND_CHILD

#define BIND_SIGNAL(sig) \
  gtk_widget_class_bind_template_callback ( \
    klass, sig)

  BIND_SIGNAL (toggles_changed);

#undef BIND_SIGNAL
}

static void
file_browser_filters_widget_init (
  FileBrowserFiltersWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "file-browser-filters");
}
