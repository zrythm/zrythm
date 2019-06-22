/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Dialog to change Marker name.
 */

#ifndef __GUI_WIDGETS_MARKER_DIALOG_H__
#define __GUI_WIDGETS_MARKER_DIALOG_H__

#include <gtk/gtk.h>

#define MARKER_DIALOG_WIDGET_TYPE \
  (marker_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MarkerDialogWidget,
  marker_dialog_widget,
  Z, MARKER_DIALOG_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct Marker Marker;

/**
 * A dialog to edit a Marker's name.
 */
typedef struct _MarkerDialogWidget
{
  GtkDialog   parent_instance;

  GtkEntry *  entry;
  GtkButton * ok;
  GtkButton * cancel;

  MarkerWidget * marker;

} MarkerDialogWidget;

/**
 * Creates the dialog.
 */
MarkerDialogWidget *
marker_dialog_widget_new (
  MarkerWidget * owner);

/**
 * @}
 */

#endif
