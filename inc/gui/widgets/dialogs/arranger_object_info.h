/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Dialog for viewing/editing port info.
 */

#ifndef __GUI_WIDGETS_DIALOGS_ARRANGER_OBJECT_INFO_H__
#define __GUI_WIDGETS_DIALOGS_ARRANGER_OBJECT_INFO_H__

#include <gtk/gtk.h>

#define ARRANGER_OBJECT_INFO_DIALOG_WIDGET_TYPE \
  (arranger_object_info_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerObjectInfoDialogWidget,
  arranger_object_info_dialog_widget,
  Z,
  ARRANGER_OBJECT_INFO_DIALOG_WIDGET,
  GtkDialog)

typedef struct ArrangerObject ArrangerObject;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The arranger_object_info dialog.
 */
typedef struct _ArrangerObjectInfoDialogWidget
{
  GtkDialog parent_instance;

  GtkLabel * name_lbl;
  GtkLabel * type_lbl;
  GtkLabel * owner_lbl;

  ArrangerObject * obj;
} ArrangerObjectInfoDialogWidget;

/**
 * Creates an arranger_object_info dialog widget and
 * displays it.
 */
ArrangerObjectInfoDialogWidget *
arranger_object_info_dialog_widget_new (
  ArrangerObject * object);

/**
 * @}
 */

#endif
