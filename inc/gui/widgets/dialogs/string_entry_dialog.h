// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
