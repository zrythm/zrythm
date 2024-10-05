// SPDX-FileCopyrightText: © 2019-2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_STRING_ENTRY_DIALOG_H__
#define __GUI_WIDGETS_STRING_ENTRY_DIALOG_H__

#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/libadwaita_wrapper.h"

#define STRING_ENTRY_DIALOG_WIDGET_TYPE (string_entry_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  StringEntryDialogWidget,
  string_entry_dialog_widget,
  Z,
  STRING_ENTRY_DIALOG_WIDGET,
  AdwMessageDialog)

typedef struct _MarkerWidget MarkerWidget;
class Marker;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * A dialog to edit a string.
 */
using StringEntryDialogWidget = struct _StringEntryDialogWidget
{
  AdwMessageDialog parent_instance;

  GtkEntry *  entry;
  GtkButton * ok;

  GenericStringGetter getter;
  GenericStringSetter setter;
};

/**
 * Creates the dialog.
 */
StringEntryDialogWidget *
string_entry_dialog_widget_new (
  const std::string  &label,
  GenericStringGetter getter,
  GenericStringSetter setter);

/**
 * @}
 */

#endif
