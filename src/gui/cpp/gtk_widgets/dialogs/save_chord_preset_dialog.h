/*
 * SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Dialog for saving chord presets.
 */

#ifndef __GUI_WIDGETS_SAVE_CHORD_PRESET_DIALOG_H__
#define __GUI_WIDGETS_SAVE_CHORD_PRESET_DIALOG_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define SAVE_CHORD_PRESET_DIALOG_WIDGET_TYPE \
  (save_chord_preset_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SaveChordPresetDialogWidget,
  save_chord_preset_dialog_widget,
  Z,
  SAVE_CHORD_PRESET_DIALOG_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The save_chord_preset dialog.
 */
typedef struct _SaveChordPresetDialogWidget
{
  GtkDialog parent_instance;

  GtkDropDown * pack_dropdown;
  GtkEntry *    preset_name_entry;
} SaveChordPresetDialogWidget;

/**
 * Creates a dialog widget and displays it.
 */
SaveChordPresetDialogWidget *
save_chord_preset_dialog_widget_new (GtkWindow * parent_window);

/**
 * @}
 */

#endif
