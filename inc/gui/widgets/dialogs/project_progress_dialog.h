/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Project progress dialog.
 */

#ifndef __GUI_WIDGETS_PROJECT_PROGRESS_DIALOG_H__
#define __GUI_WIDGETS_PROJECT_PROGRESS_DIALOG_H__

#include "gui/widgets/dialogs/generic_progress_dialog.h"

#include <gtk/gtk.h>

#define PROJECT_PROGRESS_DIALOG_WIDGET_TYPE \
  (project_progress_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ProjectProgressDialogWidget,
  project_progress_dialog_widget,
  Z,
  PROJECT_PROGRESS_DIALOG_WIDGET,
  GenericProgressDialogWidget)

typedef struct ProjectSaveData ProjectSaveData;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The project dialog.
 */
typedef struct _ProjectProgressDialogWidget
{
  GenericProgressDialogWidget parent_instance;

  ProjectSaveData * project_save_data;
} ProjectProgressDialogWidget;

/**
 * Creates an project dialog widget and displays it.
 */
ProjectProgressDialogWidget *
project_progress_dialog_widget_new (ProjectSaveData * project_save_data);

/**
 * @}
 */

#endif
