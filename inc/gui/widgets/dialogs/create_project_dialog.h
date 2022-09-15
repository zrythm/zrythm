// SPDX-FileCopyrightText: © 2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Create project dialog.
 */

#ifndef __GUI_WIDGETS_CREATE_PROJECT_DIALOG_H__
#define __GUI_WIDGETS_CREATE_PROJECT_DIALOG_H__

#include <gtk/gtk.h>

typedef struct _IdeFileChooserEntry IdeFileChooserEntry;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CREATE_PROJECT_DIALOG_WIDGET_TYPE \
  (create_project_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CreateProjectDialogWidget,
  create_project_dialog_widget,
  Z,
  CREATE_PROJECT_DIALOG_WIDGET,
  GtkDialog)

/**
 * Create project dialog.
 */
typedef struct _CreateProjectDialogWidget
{
  GtkDialog parent_instance;

  GtkEntry *            name;
  GtkBox *              fc_box;
  IdeFileChooserEntry * fc;
} CreateProjectDialogWidget;

CreateProjectDialogWidget *
create_project_dialog_widget_new (void);

/**
 * @}
 */

#endif
