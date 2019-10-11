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
 * \file Ruler parent class.
 */

#ifndef __GUI_WIDGETS_RULER_H__
#define __GUI_WIDGETS_RULER_H__

#include "utils/ui.h"

#include <gtk/gtk.h>

#define DEFAULT_ZOOM_LEVEL 1.0f

#define RULER_WIDGET_TYPE \
  (ruler_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (
  RulerWidget,
  ruler_widget,
  Z, RULER_WIDGET,
  GtkOverlay)

#define RULER_WIDGET_GET_PRIVATE(self) \
  RulerWidgetPrivate * rw_prv = \
    ruler_widget_get_private ( \
      (RulerWidget *) (self));

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

typedef struct Position Position;
typedef struct _RulerMarkerWidget RulerMarkerWidget;

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

typedef struct
{
  RulerMarkerWidget *      playhead;
  GtkDrawingArea *         bg;
  double                   px_per_beat;
  double                   px_per_bar;
  double                   px_per_sixteenth;
  double                   px_per_tick;
  double                   total_px;

  /**
   * Dragging playhead or creating range, etc.
   */
  UiOverlayAction          action;

  /** For dragging. */
  double                   start_x;
  double                   last_offset_x;

  GtkGestureDrag *         drag;
  GtkGestureMultiPress *   multipress;

  /** Target acting upon. */
  RWTarget                 target;

  /**
   * If shift was held down during the press.
   */
  int                      shift_held;

  /** FIXME move somewhere else */
  double                   zoom_level;
} RulerWidgetPrivate;

typedef struct _RulerWidgetClass
{
  GtkOverlayClass    parent_class;
} RulerWidgetClass;

RulerWidgetPrivate *
ruler_widget_get_private (RulerWidget * self);

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

void
ruler_widget_refresh (RulerWidget * self);

#endif
