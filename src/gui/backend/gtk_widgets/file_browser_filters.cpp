// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/resources.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/file_browser_filters.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (FileBrowserFiltersWidget, file_browser_filters_widget, GTK_TYPE_BOX)

static void
toggles_changed (GtkToggleButton * btn, FileBrowserFiltersWidget * self)
{
  if (gtk_toggle_button_get_active (btn))
    {
      /* block signals, unset all, unblock */
      g_signal_handlers_block_by_func (
        self->toggle_audio, (gpointer) toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_midi, (gpointer) toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_presets, (gpointer) toggles_changed, self);

      if (btn == self->toggle_audio)
        {
          g_settings_set_enum (
            S_UI_FILE_BROWSER, "filter",
            ENUM_VALUE_TO_INT (FileBrowserFilterType::FILE_BROWSER_FILTER_AUDIO));
          gtk_toggle_button_set_active (self->toggle_midi, 0);
          gtk_toggle_button_set_active (self->toggle_presets, 0);
        }
      else if (btn == self->toggle_midi)
        {
          g_settings_set_enum (
            S_UI_FILE_BROWSER, "filter",
            ENUM_VALUE_TO_INT (FileBrowserFilterType::FILE_BROWSER_FILTER_MIDI));
          gtk_toggle_button_set_active (self->toggle_audio, 0);
          gtk_toggle_button_set_active (self->toggle_presets, 0);
        }
      else if (btn == self->toggle_presets)
        {
          g_settings_set_enum (
            S_UI_FILE_BROWSER, "filter",
            ENUM_VALUE_TO_INT (
              FileBrowserFilterType::FILE_BROWSER_FILTER_PRESET));
          gtk_toggle_button_set_active (self->toggle_midi, false);
          gtk_toggle_button_set_active (self->toggle_audio, false);
        }

      g_signal_handlers_unblock_by_func (
        self->toggle_audio, (gpointer) toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_midi, (gpointer) toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_presets, (gpointer) toggles_changed, self);
    }
  else
    {
      g_settings_set_enum (
        S_UI_FILE_BROWSER, "filter",
        ENUM_VALUE_TO_INT (FileBrowserFilterType::FILE_BROWSER_FILTER_NONE));
    }

  self->refilter_files ();
}

/**
 * Sets up a FileBrowserFiltersWidget.
 */
void
file_browser_filters_widget_setup (
  FileBrowserFiltersWidget * self,
  GenericCallback            refilter_files_cb)
{
  self->refilter_files = refilter_files_cb;
}

static void
file_browser_filters_widget_class_init (FileBrowserFiltersWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "file_browser_filters.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, FileBrowserFiltersWidget, x)

  BIND_CHILD (toggle_audio);
  BIND_CHILD (toggle_midi);
  BIND_CHILD (toggle_presets);

#undef BIND_CHILD

#define BIND_SIGNAL(sig) gtk_widget_class_bind_template_callback (klass, sig)

  BIND_SIGNAL (toggles_changed);

#undef BIND_SIGNAL
}

static void
file_browser_filters_widget_init (FileBrowserFiltersWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (GTK_WIDGET (self), "file-browser-filters");
}
