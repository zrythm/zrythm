// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * File browser widget.
 */

#ifndef __GUI_WIDGETS_FILE_BROWSER_H__
#define __GUI_WIDGETS_FILE_BROWSER_H__

#include "dsp/supported_file.h"

#include <gtk/gtk.h>

#define FILE_BROWSER_WIDGET_TYPE \
  (file_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FileBrowserWidget,
  file_browser_widget,
  Z,
  FILE_BROWSER_WIDGET,
  GtkBox)

typedef struct _FileAuditionerControlsWidget
  FileAuditionerControlsWidget;
typedef struct _FileBrowserFiltersWidget
  FileBrowserFiltersWidget;
typedef struct _WrappedObjectWithChangeSignal
  WrappedObjectWithChangeSignal;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_FILE_BROWSER MW_RIGHT_DOCK_EDGE->file_browser

/**
 * The file browser for samples, MIDI files, etc.
 */
typedef struct _FileBrowserWidget
{
  GtkBox     parent_instance;
  GtkBox *   browser_top;
  GtkBox *   browser_bot;
  GtkLabel * file_info;
  ZFileType  selected_type;

  FileBrowserFiltersWidget * filters_toolbar;

  /** The file chooser. */
  GtkBox *               file_chooser_box;
  GtkFileChooserWidget * file_chooser;

  FileAuditionerControlsWidget * auditioner_controls;

  /** Currently selected file. */
  WrappedObjectWithChangeSignal * selected_file;

  /**
   * A little hack to get the paned position to
   * get set from the gsettings when first created.
   *
   * Had problems where the position was quickly
   * overwritten by something random within 300 ms
   * of widget creation.
   */
  int    start_saving_pos;
  int    first_time_position_set;
  gint64 first_time_position_set_time;
} FileBrowserWidget;

/**
 * Creates a new FileBrowserWidget.
 */
FileBrowserWidget *
file_browser_widget_new (void);

/**
 * @}
 */

#endif
