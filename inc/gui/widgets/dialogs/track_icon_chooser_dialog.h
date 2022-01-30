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
 * Object color chooser dialog.
 */

#ifndef __GUI_WIDGETS_TRACK_ICON_CHOOSER_DIALOG_H__
#define __GUI_WIDGETS_TRACK_ICON_CHOOSER_DIALOG_H__

#include <gtk/gtk.h>

typedef Track Track;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Dialog for choosing colors of objects like tracks
 * and regions.
 */
typedef struct TrackIconChooserDialogWidget
{
  GtkDialog *        dialog;

  GtkTreeModel *     icon_model;
  GtkIconView *      icon_view;

  char *             selected_icon;

  /** Track. */
  Track *            track;
} TrackIconChooserDialogWidget;

/**
 * Creates a new dialog.
 */
TrackIconChooserDialogWidget *
track_icon_chooser_dialog_widget_new (
  Track * track);

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @return Whether the icon was set or not.
 */
bool
track_icon_chooser_dialog_widget_run (
  TrackIconChooserDialogWidget * self);

/**
 * @}
 */

#endif
