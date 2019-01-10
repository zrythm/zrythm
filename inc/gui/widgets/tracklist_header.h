/*
 * gui/widgets/tracklist_header.h - The box where ruler and
 *   tracklist meet
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_TRACKLIST_HEADER_H__
#define __GUI_WIDGETS_TRACKLIST_HEADER_H__

#include <gtk/gtk.h>

#define TRACKLIST_HEADER_WIDGET_TYPE \
  (tracklist_header_widget_get_type ())
G_DECLARE_FINAL_TYPE (TracklistHeaderWidget,
                      tracklist_header_widget,
                      Z,
                      TRACKLIST_HEADER_WIDGET,
                      GtkGrid)

typedef struct _TracklistHeaderWidget
{
  GtkGrid                parent_instance;
} TracklistHeaderWidget;

#endif
