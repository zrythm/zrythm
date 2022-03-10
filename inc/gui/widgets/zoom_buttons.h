/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#ifndef __GUI_WIDGETS_ZOOM_BUTTONS_H__
#define __GUI_WIDGETS_ZOOM_BUTTONS_H__

#include <gtk/gtk.h>

#define ZOOM_BUTTONS_WIDGET_TYPE \
  (zoom_buttons_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ZoomButtonsWidget, zoom_buttons_widget,
  Z, ZOOM_BUTTONS_WIDGET, GtkBox)

/**
 * Zoom buttons for toolbars.
 */
typedef struct _ZoomButtonsWidget
{
  GtkBox         parent_instance;
  GtkButton *    zoom_in;
  GtkButton *    zoom_out;
  GtkButton *    original_size;
  GtkButton *    best_fit;
} ZoomButtonsWidget;

void
zoom_buttons_widget_setup (
  ZoomButtonsWidget * self,
  bool                timeline);

#endif
