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
 * ArrangerObject related functions for the GUI.
 */

#ifndef __GUI_WIDGETS_ARRANGER_OBJECT_H__
#define __GUI_WIDGETS_ARRANGER_OBJECT_H__

#include "gui/backend/arranger_object.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Returns if the current position is for resizing
 * L.
 */
int
arranger_object_is_resize_l (
  ArrangerObject * self,
  const int        x);

/**
 * Returns if the current position is for resizing
 * R.
 */
int
arranger_object_is_resize_r (
  ArrangerObject * self,
  const int        x);

/**
 * Returns if the current position is for resizing
 * up (eg, Velocity).
 */
int
arranger_object_is_resize_up (
  ArrangerObject * self,
  const int        x,
  const int        y);

/**
 * Returns if the current position is for resizing
 * loop.
 */
int
arranger_object_is_resize_loop (
  ArrangerObject * self,
  const int        y);

/**
 * Returns if arranger_object widgets should show
 * cut lines.
 *
 * To be used to set the arranger_object's
 * "show_cut".
 *
 * @param alt_pressed Whether alt is currently
 *   pressed.
 */
int
arranger_object_should_show_cut_lines (
  ArrangerObject * self,
  int              alt_pressed);

void
arranger_object_set_full_rectangle (
  ArrangerObject * self,
  ArrangerWidget * arranger);

/**
 * Gets the draw rectangle based on the given
 * full rectangle of the arranger object.
 *
 * @param parent_rect The current arranger
 *   rectangle.
 * @param full_rect The object's full rectangle.
 *   This will usually be ArrangerObject->full_rect,
 *   unless drawing in a lane (for Region's).
 * @param draw_rect The rectangle will be set
 *   here.
 */
void
arranger_object_get_draw_rectangle (
  ArrangerObject * self,
  GdkRectangle *   parent_rect,
  GdkRectangle *   full_rect,
  GdkRectangle *   draw_rect);

/**
 * Draws the given object.
 *
 * To be called from the arranger's draw callback.
 *
 * @param cr Cairo context of the arranger.
 * @param rect Rectangle in the arranger.
 */
void
arranger_object_draw (
  ArrangerObject * self,
  ArrangerWidget * arranger,
  cairo_t *        cr,
  GdkRectangle *   rect);

#if 0
/**
* ArrangerObject widget base private.
*/
typedef struct _ArrangerObjectWidgetPrivate
{
  /** ArrangerObject associated with this widget. */
  ArrangerObject *   arranger_object;

  /** If cursor is at resizing up. */
  int                resize_up;

  /** If cursor is at resizing L. */
  int                resize_l;

  /** If cursor is at resizing R. */
  int                resize_r;

  /**
   * If resize cursor should be a loop.
   *
   * This only applies if either one of the above
   * is true.
   */
  int                resize_loop;

  GtkDrawingArea *   drawing_area;

  /** Show a cut line or not. */
  int                show_cut;

  /** Last hover position. */
  int                hover_x;

  /** A tooltip window to show while dragging
   * the object. */
  GtkWindow *        tooltip_win;
  GtkLabel *         tooltip_label;

  /** Set to 1 to redraw. */
  int                redraw;

  /**
   * Last rectangle used to draw in.
   *
   * If this is different from the current one,
   * the widget should be redrawn.
   */
  GdkRectangle       last_rect;

} ArrangerObjectWidgetPrivate;

void
arranger_object_widget_force_redraw (
  ArrangerObjectWidget * self);

/**
 * Draws the cut line if in cut mode.
 */
void
arranger_object_widget_draw_cut_line (
  ArrangerObjectWidget * self,
  cairo_t *              cr,
  GdkRectangle *         rect);

/**
 * Returns if the ArrangerObjectWidget should
 * be redrawn.
 *
 * @param rect The rectangle for this draw cycle.
 */
int
arranger_object_widget_should_redraw (
  ArrangerObjectWidget * self,
  GdkRectangle *         rect);

/**
 * Updates the normal tooltip of the widget, and
 * shows the custom tooltip window if show is 1.
 */
void
arranger_object_widget_update_tooltip (
  ArrangerObjectWidget * self,
  int                    show);

/**
 * To be called after redrawing an arranger object.
 */
void
arranger_object_widget_post_redraw (
  ArrangerObjectWidget * self,
  GdkRectangle *         rect);
#endif

/**
 * @}
 */

#endif
