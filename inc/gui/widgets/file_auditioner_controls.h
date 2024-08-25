/*
 * SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * File auditioner controls.
 */

#ifndef __GUI_WIDGETS_FILE_AUDITIONER_CONTROLS_H__
#define __GUI_WIDGETS_FILE_AUDITIONER_CONTROLS_H__

#include "zrythm-config.h"

#include "io/file_descriptor.h"

#include "gtk_wrapper.h"

#define FILE_AUDITIONER_CONTROLS_WIDGET_TYPE \
  (file_auditioner_controls_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FileAuditionerControlsWidget,
  file_auditioner_controls_widget,
  Z,
  FILE_AUDITIONER_CONTROLS_WIDGET,
  GtkBox)

typedef struct _VolumeWidget                  VolumeWidget;
typedef struct _WrappedObjectWithChangeSignal WrappedObjectWithChangeSignal;

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
using FileAuditionerControlsWidget = struct _FileAuditionerControlsWidget
{
  GtkBox parent_instance;

  /**
   * True if these controls are for files.
   *
   * This adds some more settings like the ability
   * to show hidden files.
   */
  bool for_files;

  GtkButton *     play_btn;
  GtkButton *     stop_btn;
  GtkMenuButton * file_settings_btn;
  VolumeWidget *  volume;

  GtkDropDown * instrument_dropdown;

  /** Callbacks. */
  GtkWidget *        owner;
  SelectedFileGetter selected_file_getter;
  GenericCallback    refilter_files;
};

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
