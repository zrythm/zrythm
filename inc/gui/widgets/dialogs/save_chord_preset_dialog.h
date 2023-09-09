/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Dialog for saving chord presets.
 */

#ifndef __GUI_WIDGETS_SAVE_CHORD_PRESET_DIALOG_H__
#define __GUI_WIDGETS_SAVE_CHORD_PRESET_DIALOG_H__

#include <gtk/gtk.h>

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
