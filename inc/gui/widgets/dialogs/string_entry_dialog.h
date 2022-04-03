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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Dialog to change a string.
 */

#ifndef __GUI_WIDGETS_STRING_ENTRY_DIALOG_H__
#define __GUI_WIDGETS_STRING_ENTRY_DIALOG_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#define STRING_ENTRY_DIALOG_WIDGET_TYPE \
  (string_entry_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  StringEntryDialogWidget,
  string_entry_dialog_widget,
  Z,
  STRING_ENTRY_DIALOG_WIDGET,
  GtkDialog)

typedef struct _MarkerWidget MarkerWidget;
typedef struct Marker        Marker;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * A dialog to edit a string.
 */
typedef struct _StringEntryDialogWidget
{
  GtkDialog parent_instance;

  GtkEntry *  entry;
  GtkButton * ok;
  GtkButton * cancel;
  GtkLabel *  label;

  GenericStringGetter getter;
  GenericStringSetter setter;
  void *              obj;

} StringEntryDialogWidget;

/**
 * Creates the dialog.
 */
StringEntryDialogWidget *
string_entry_dialog_widget_new (
  const char *        label,
  void *              obj,
  GenericStringGetter getter,
  GenericStringSetter setter);

/**
 * Runs a new dialog and returns the string, or
 * NULL if cancelled.
 */
char *
string_entry_dialog_widget_new_return_string (
  const char * label);

/**
 * @}
 */

#endif
