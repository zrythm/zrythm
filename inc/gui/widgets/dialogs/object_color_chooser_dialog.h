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
 * Objejct color chooser dialog.
 */

#ifndef __GUI_WIDGETS_OBJECT_COLOR_CHOOSER_DIALOG_H__
#define __GUI_WIDGETS_OBJECT_COLOR_CHOOSER_DIALOG_H__

#include <gtk/gtk.h>

#define OBJECT_COLOR_CHOOSER_DIALOG_WIDGET_TYPE \
  (object_color_chooser_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ObjectColorChooserDialogWidget,
  object_color_chooser_dialog_widget,
  Z, OBJECT_COLOR_CHOOSER_DIALOG_WIDGET,
  GtkColorChooserDialog)

typedef Track Track;
typedef ZRegion ZRegion;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Dialog for choosing colors of objects like tracks
 * and regions.
 */
typedef struct _ObjectColorChooserDialogWidget
{
  GtkColorChooserDialog parent_instance;

  /** Track, if for track. */
  Track *               track;

  /* Region, if for region. */
  ZRegion *             region;
} ObjectColorChooserDialogWidget;

/**
 * Creates a new dialog.
 */
ObjectColorChooserDialogWidget *
object_color_chooser_dialog_widget_new_for_track (
  Track * track);

/**
 * Creates a new dialog.
 */
ObjectColorChooserDialogWidget *
object_color_chooser_dialog_widget_new_for_region (
  ZRegion * region);

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @return Whether the color was set or not.
 */
bool
object_color_chooser_dialog_widget_run (
  ObjectColorChooserDialogWidget * self);

/**
 * @}
 */

#endif
