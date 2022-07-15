/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_OBJECT_COLOR_CHOOSER_DIALOG_H__
#define __GUI_WIDGETS_OBJECT_COLOR_CHOOSER_DIALOG_H__

#include <gtk/gtk.h>

typedef struct Track               Track;
typedef struct ZRegion             ZRegion;
typedef struct TracklistSelections TracklistSelections;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @param track Track, if track.
 * @param sel TracklistSelections, if multiple
 *   tracks.
 * @param region ZRegion, if region.
 *
 * @return Whether the color was set or not.
 */
bool
object_color_chooser_dialog_widget_run (
  GtkWindow *           parent,
  Track *               track,
  TracklistSelections * sel,
  ZRegion *             region);

/**
 * @}
 */

#endif
