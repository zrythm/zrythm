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

#ifndef __GUI_WIDGETS_VOLUME_H__
#define __GUI_WIDGETS_VOLUME_H__

#include <gtk/gtk.h>

typedef struct Port Port;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define VOLUME_WIDGET_TYPE (volume_widget_get_type ())
G_DECLARE_FINAL_TYPE (VolumeWidget, volume_widget, Z, VOLUME_WIDGET, GtkDrawingArea)

typedef struct _VolumeWidget
{
  GtkDrawingArea parent_instance;

  /** Control port to change. */
  Port * port;

  bool hover;

  GtkGestureDrag * drag;

  double last_x;
  double last_y;
} VolumeWidget;

void
volume_widget_setup (VolumeWidget * self, Port * port);

VolumeWidget *
volume_widget_new (Port * port);

/**
 * @}
 */

#endif
