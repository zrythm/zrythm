/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#define ARRANGER_OBJECT_FADE_POINT_WIDTH 12
#define ARRANGER_OBJECT_FADE_POINT_HALFWIDTH 6

/**
 * Returns if the current position is for moving the
 * fade in/out mark (timeline only).
 *
 * @param in True for fade in, false for fade out.
 * @param x X in local coordinates.
 * @param only_handle Whether to only check if this
 *   is inside the fade handle. If this is false,
 *   \ref only_outer will be considered.
 * @param only_outer Whether to only check if this
 *   is inside the fade's outer (unplayed) region.
 *   If this is false, the whole fade area will
 *   be considered.
 * @param check_lane Whether to check the lane
 *   region instead of the main one (if region).
 */
bool
arranger_object_is_fade (
  ArrangerObject * self,
  bool             in,
  const int        x,
  int              y,
  bool             only_handle,
  bool             only_outer,
  bool             check_lane);

#define arranger_object_is_fade_in( \
  self, x, y, only_handle, only_outer) \
  arranger_object_is_fade ( \
    self, true, x, y, only_handle, only_outer, true) \
    || arranger_object_is_fade ( \
      self, true, x, y, only_handle, only_outer, false)

#define arranger_object_is_fade_out( \
  self, x, y, only_handle, only_outer) \
  arranger_object_is_fade ( \
    self, false, x, y, only_handle, only_outer, true) \
    || arranger_object_is_fade ( \
      self, false, x, y, only_handle, only_outer, false)

/**
 * Returns if the current position is for resizing
 * L.
 *
 * @param x X in local coordinates.
 */
NONNULL PURE bool
arranger_object_is_resize_l (
  ArrangerObject * self,
  const int        x);

/**
 * Returns if the current position is for resizing
 * R.
 *
 * @param x X in local coordinates.
 */
NONNULL bool
arranger_object_is_resize_r (
  ArrangerObject * self,
  const int        x);

/**
 * Returns if the current position is for resizing
 * up (eg, Velocity).
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
bool
arranger_object_is_resize_up (
  ArrangerObject * self,
  const int        x,
  const int        y);

/**
 * Returns if the current position is for resizing
 * loop.
 *
 * @param y Y in local coordinates.
 */
bool
arranger_object_is_resize_loop (
  ArrangerObject * self,
  const int        y);

/**
 * Returns if the current position is for renaming
 * the object.
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
NONNULL bool
arranger_object_is_rename (
  ArrangerObject * self,
  const int        x,
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
bool
arranger_object_should_show_cut_lines (
  ArrangerObject * self,
  bool             alt_pressed);

/**
 * Gets the full rectangle for a linked object.
 */
int
arranger_object_get_full_rect_x_for_region_child (
  ArrangerObject * self,
  ZRegion *        region,
  GdkRectangle *   full_rect);

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
 *
 * @return Whether the draw rect is visible.
 */
int
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
 * @param rect Rectangle in the arranger.
 */
void
arranger_object_draw (
  ArrangerObject * self,
  ArrangerWidget * arranger,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect);

/**
 * Returns if the cached object should be visible,
 * ie, while copy- moving (ctrl+drag) we want to
 * show both the object at its original position
 * and the current object.
 *
 * This refers to the object at its original
 * position (called "transient").
 *
 * @param arranger Owner arranger. Should be passed
 *   to speed up calculation.
 */
NONNULL_ARGS (1)
OPTIMIZE_O3 bool arranger_object_should_orig_be_visible (
  ArrangerObject * self,
  ArrangerWidget * arranger);

/**
 * @}
 */

#endif
