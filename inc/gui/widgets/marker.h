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
 * Marker widget.
 */

#ifndef __GUI_WIDGETS_MARKER_H__
#define __GUI_WIDGETS_MARKER_H__

#include "gui/widgets/arranger_object.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MARKER_NAME_FONT "Sans SemiBold 8"
#define MARKER_NAME_PADDING 2

/**
 * Recreates the pango layouts for drawing.
 */
void
marker_recreate_pango_layouts (
  Marker * self);

/**
 *
 * @param cr Cairo context of the arranger.
 * @param rect Rectangle in the arranger.
 */
void
marker_draw (
  Marker *       self,
  cairo_t *      cr,
  GdkRectangle * rect);

#if 0

#define MARKER_WIDGET_TRIANGLE_W 10

/**
 * Widget for markers inside the MarkerTrack.
 */
typedef struct _MarkerWidget
{
  ArrangerObjectWidget parent_instance;
  Marker *             marker;

  /** For double click. */
  GtkGestureClick * mp;

  PangoLayout *        layout;

  /** Cache text H extents and W extents for
   * the widget. */
  int                  textw;
  int                  texth;
} MarkerWidget;

/**
 * Creates a marker widget.
 */
MarkerWidget *
marker_widget_new (
  Marker * marker);

void
marker_widget_recreate_pango_layouts (
  MarkerWidget * self);

#endif

/**
 * @}
 */

#endif
