/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_FILE_AUDITIONER_CONTROLS_H__
#define __GUI_WIDGETS_FILE_AUDITIONER_CONTROLS_H__

#include "zrythm-config.h"

#include "audio/supported_file.h"

#include <gtk/gtk.h>

#define FILE_AUDITIONER_CONTROLS_WIDGET_TYPE \
  (file_auditioner_controls_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FileAuditionerControlsWidget,
  file_auditioner_controls_widget,
  Z, FILE_AUDITIONER_CONTROLS_WIDGET,
  GtkBox)

typedef struct _VolumeWidget VolumeWidget;
typedef struct _WrappedObjectWithChangeSignal
  WrappedObjectWithChangeSignal;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef WrappedObjectWithChangeSignal * (*SelectedFileGetter) (
  GtkWidget * widget);

/**
 * File auditioner controls used in file browsers.
 */
typedef struct _FileAuditionerControlsWidget
{
  GtkBox               parent_instance;

  /**
   * True if these controls are for files.
   *
   * This adds some more settings like the ability
   * to show hidden files.
   */
  bool                 for_files;

  GtkButton *          play_btn;
  GtkButton *          stop_btn;
  GtkMenuButton *      file_settings_btn;
  VolumeWidget *       volume;

  GtkDropDown *        instrument_dropdown;

  /** Callbacks. */
  GtkWidget *          owner;
  SelectedFileGetter   selected_file_getter;
  GenericCallback      refilter_files;
} FileAuditionerControlsWidget;

/**
 * Sets up a FileAuditionerControlsWidget.
 */
void
file_auditioner_controls_widget_setup (
  FileAuditionerControlsWidget * self,
  GtkWidget *                    owner,
  bool                           for_files,
  SelectedFileGetter             selected_file_getter,
  GenericCallback                refilter_files_cb);

/**
 * @}
 */

#endif
