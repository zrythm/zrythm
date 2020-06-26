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
 * Ruler parent class.
 */

#ifndef __GUI_WIDGETS_RULER_H__
#define __GUI_WIDGETS_RULER_H__

#include "utils/ui.h"

#include <gtk/gtk.h>

#define RW_DEFAULT_ZOOM_LEVEL 1.0f

#define RW_RULER_MARKER_SIZE 8
#define RW_CUE_MARKER_HEIGHT 12
#define RW_CUE_MARKER_WIDTH 7
#define RW_PLAYHEAD_TRIANGLE_WIDTH 12
#define RW_PLAYHEAD_TRIANGLE_HEIGHT 8
#define RW_RANGE_HEIGHT_DIVISOR 4

/**
 * Minimum number of pixels between beat lines.
 */
#define RW_PX_TO_HIDE_BEATS 40.0

#define RULER_WIDGET_TYPE \
  (ruler_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RulerWidget,
  ruler_widget,
  Z, RULER_WIDGET,
  GtkDrawingArea)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct Position Position;

/**
 * Pixels to draw between each beat, before being
 * adjusted for zoom.
 *
 * Used by the ruler and timeline.
 */
#define DEFAULT_PX_PER_TICK 0.03

/**
 * Pixels to put before 1st bar.
 */
#define SPACE_BEFORE_START 10
#define SPACE_BEFORE_START_F 10.f
#define SPACE_BEFORE_START_D 10.0

#define MIN_ZOOM_LEVEL 0.05
#define MAX_ZOOM_LEVEL 400.

/**
 * The ruler widget target acting upon.
 */
typedef enum RWTarget
{
  RW_TARGET_PLAYHEAD,
  RW_TARGET_LOOP_START,
  RW_TARGET_LOOP_END,
  RW_TARGET_CLIP_START,
  RW_TARGET_RANGE, ///< for timeline only
} RWTarget;

typedef enum RulerWidgetType
{
  RULER_WIDGET_TYPE_TIMELINE,
  RULER_WIDGET_TYPE_EDITOR,
} RulerWidgetType;

/**
 * Range type.
 */
typedef enum RulerWidgetRangeType
{
  /** Range start. */
  RW_RANGE_START,
  /** Whole range. */
  RW_RANGE_FULL,
  /** Range end. */
  RW_RANGE_END,
} RulerWidgetRangeType;

typedef struct _RulerWidget
{
  GtkDrawingArea    parent_instance;

  RulerWidgetType   type;

  double            px_per_beat;
  double            px_per_bar;
  double            px_per_sixteenth;
  double            px_per_tick;
  double            total_px;

  /**
   * Dragging playhead or creating range, etc.
   */
  UiOverlayAction   action;

  /** For dragging. */
  double            start_x;
  double            start_y;
  double            last_offset_x;

  GtkGestureDrag *  drag;
  GtkGestureMultiPress * multipress;

  /** Target acting upon. */
  RWTarget          target;

  /**
   * If shift was held down during the press.
   */
  int               shift_held;

  /** FIXME move somewhere else */
  double            zoom_level;

  /** Px the playhead was last drawn at, so we can
   * redraw this and the new px only when the
   * playhead changes position. */
  int               last_playhead_px;

  /** Set to 1 to redraw. */
  int               redraw;

  /** Whether range1 was before range2 at drag
   * start. */
  int               range1_first;

  /** Set to 1 between drag begin and drag end. */
  int               dragging;

  /**
   * Set on drag begin.
   *
   * Useful for moving range.
   */
  Position          range1_start_pos;
  Position          range2_start_pos;

  /**
   * Last position the playhead was set to.
   *
   * This is used for setting the cue point on
   * drag end.
   */
  Position          last_set_pos;

  cairo_t *         cached_cr;

  cairo_surface_t * cached_surface;

  /** Rectangle in the last call. */
  GdkRectangle      last_rect;
} RulerWidget;

/**
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * Returns if the zoom level was set or not.
 */
int
ruler_widget_set_zoom_level (
  RulerWidget * self,
  double        zoom_level);

/**
 * Returns the beat interval for drawing vertical
 * lines.
 */
int
ruler_widget_get_beat_interval (
  RulerWidget * self);

/**
 * Returns the sixteenth interval for drawing
 * vertical lines.
 */
int
ruler_widget_get_sixteenth_interval (
  RulerWidget * self);

/**
 * Queues a redraw of the whole visible ruler.
 */
void
ruler_widget_redraw_whole (
  RulerWidget * self);

/**
 * Only redraws the playhead part.
 */
void
ruler_widget_redraw_playhead (
  RulerWidget * self);

bool
ruler_widget_is_range_hit (
  RulerWidget *        self,
  RulerWidgetRangeType type,
  double               x,
  double               y);

void
ruler_widget_refresh (
  RulerWidget * self);

/**
 * @}
 */

#endif
