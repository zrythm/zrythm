/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Arranger background.
 */

#if 0

#ifndef __GUI_WIDGETS_ARRANGER_BG_H__
#define __GUI_WIDGETS_ARRANGER_BG_H__

#include <gtk/gtk.h>

#define ARRANGER_BG_WIDGET_TYPE \
  (arranger_bg_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (ArrangerBgWidget,
                          arranger_bg_widget,
                          Z,
                          ARRANGER_BG_WIDGET,
                          GtkDrawingArea)

typedef struct _ArrangerWidget ArrangerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define ARRANGER_BG_WIDGET_GET_PRIVATE(self) \
  ArrangerBgWidgetPrivate * ab_prv = \
    arranger_bg_widget_get_private ( \
      Z_ARRANGER_BG_WIDGET (self));

typedef struct _RulerWidget RulerWidget;

typedef struct
{
  int                     total_px;
  double                  start_x; ///< for dragging
  GtkGestureDrag *        drag;
  GtkGestureMultiPress  * multipress;
  RulerWidget *           ruler; ///< associated ruler
  /** Owner arranger. */
  ArrangerWidget *        arranger;

  /** Set to 1 to redraw. */
  int               redraw;

  cairo_t *               cached_cr;

  cairo_surface_t *       cached_surface;

  /** Rectangle in the last call. */
  GdkRectangle            last_rect;
} ArrangerBgWidgetPrivate;

typedef struct _ArrangerBgWidgetClass
{
  GtkDrawingAreaClass    parent_class;
} ArrangerBgWidgetClass;

/**
 * Gets the ArrangerBgWidgetPrivate.
 */
ArrangerBgWidgetPrivate *
arranger_bg_widget_get_private (
  ArrangerBgWidget * self);

/**
 * Refreshes the widget.
 */
void
arranger_bg_widget_refresh (
  ArrangerBgWidget * self);

/**
 * Draws the selection in its background.
 *
 * Should only be called by the bg widgets when drawing.
 */
void
arranger_bg_widget_draw_selections (
  ArrangerWidget * self,
  cairo_t *        cr,
  GdkRectangle *   rect);

/**
 * @}
 */

#endif
#endif
