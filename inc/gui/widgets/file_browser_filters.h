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
 * File auditioner controls.
 */

#ifndef __GUI_WIDGETS_FILE_BROWSER_FILTERS_H__
#define __GUI_WIDGETS_FILE_BROWSER_FILTERS_H__

#include "zrythm-config.h"

#include "dsp/supported_file.h"
#include "utils/types.h"

#include <gtk/gtk.h>

#define FILE_BROWSER_FILTERS_WIDGET_TYPE \
  (file_browser_filters_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FileBrowserFiltersWidget,
  file_browser_filters_widget,
  Z,
  FILE_BROWSER_FILTERS_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum FileBrowserFilterType
{
  FILE_BROWSER_FILTER_NONE,
  FILE_BROWSER_FILTER_AUDIO,
  FILE_BROWSER_FILTER_MIDI,
  FILE_BROWSER_FILTER_PRESET,
} FileBrowserFilterType;

/**
 * File auditioner controls used in file browsers.
 */
typedef struct _FileBrowserFiltersWidget
{
  GtkBox parent_instance;

  GtkToggleButton * toggle_audio;
  GtkToggleButton * toggle_midi;
  GtkToggleButton * toggle_presets;

  /** Callbacks. */
  GtkWidget *     owner;
  GenericCallback refilter_files;
} FileBrowserFiltersWidget;

/**
 * Sets up a FileBrowserFiltersWidget.
 */
void
file_browser_filters_widget_setup (
  FileBrowserFiltersWidget * self,
  GtkWidget *                owner,
  GenericCallback            refilter_files_cb);

/**
 * @}
 */

#endif
