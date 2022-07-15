// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Scripting window.
 */

#ifndef __GUI_WIDGETS_SCRIPTING_DIALOG_H__
#define __GUI_WIDGETS_SCRIPTING_DIALOG_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

#define SCRIPTING_DIALOG_WIDGET_TYPE \
  (scripting_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ScriptingDialogWidget,
  scripting_dialog_widget,
  Z,
  SCRIPTING_DIALOG_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The export dialog.
 */
typedef struct _ScriptingDialogWidget
{
  GtkDialog parent_instance;

  AdwComboRow * lang_select_combo_row;

  GtkButton *       execute_btn;
  GtkLabel *        output;
  GtkViewport *     source_viewport;
  GtkSourceView *   editor;
  GtkSourceBuffer * buffer;
} ScriptingDialogWidget;

/**
 * Creates a bounce dialog.
 */
ScriptingDialogWidget *
scripting_dialog_widget_new (void);

/**
 * @}
 */

#endif
