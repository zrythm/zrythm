/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Create project dialog.
 */

#ifndef __GUI_WIDGETS_CREATE_PROJECT_DIALOG_H__
#define __GUI_WIDGETS_CREATE_PROJECT_DIALOG_H__

#include <gtk/gtk.h>

typedef struct _FileChooserButtonWidget FileChooserButtonWidget;

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

  GtkButton *               ok;
  GtkEntry *                name;
  FileChooserButtonWidget * fc;
} CreateProjectDialogWidget;

CreateProjectDialogWidget *
create_project_dialog_widget_new (void);

/**
 * @}
 */

#endif
