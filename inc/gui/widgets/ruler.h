/*
 * inc/gui/widgets/ruler.h - Ruler
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_RULER_H__
#define __GUI_WIDGETS_RULER_H__

#include <gtk/gtk.h>

#define DEFAULT_ZOOM_LEVEL 1.0f

#define RULER_WIDGET_TYPE \
  (ruler_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (RulerWidget,
                          ruler_widget,
                          Z,
                          RULER_WIDGET,
                          GtkOverlay)

#define RULER_WIDGET_GET_PRIVATE(self) \
  RulerWidgetPrivate * rw_prv = \
    ruler_widget_get_private (Z_RULER_WIDGET (self));

/**
 * pixels to draw between each beat,
 * before being adjusted for zoom.
 * used by the ruler and timeline
 */
#define DEFAULT_PX_PER_TICK 0.03f

#define SPACE_BEFORE_START 10 /* pixels to put before 1st bar */

#define MIN_ZOOM_LEVEL 0.2f
#define MAX_ZOOM_LEVEL 60.f

typedef struct Position Position;
typedef struct _RulerPlayheadWidget RulerPlayheadWidget;

typedef struct
{
  RulerPlayheadWidget *    playhead;
  GtkDrawingArea *         bg;
  unsigned int             px_per_beat;
  unsigned int             px_per_bar;
  unsigned int             px_per_sixteenth;
  double                   px_per_tick;
  unsigned int             total_px;
  double                   start_x; ///< for dragging

  /**
   * Zoom level.
   */
  double                   zoom_level;

  GtkGestureDrag *         drag;
  GtkGestureMultiPress *   multipress;
} RulerWidgetPrivate;

typedef struct _RulerWidgetClass
{
  GtkOverlayClass    parent_class;
} RulerWidgetClass;

/**
 * Turns px position into position.
 */
void
ruler_widget_px_to_pos (
  RulerWidget * self,
  Position * pos, ///< position to fill in
  int      px); ///< pixels

int
ruler_widget_pos_to_px (
  RulerWidget * self,
  Position * pos);

RulerWidgetPrivate *
ruler_widget_get_private (RulerWidget * self);

/**
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * Returns if the zoom level was set or not.
 */
int
ruler_widget_set_zoom_level (RulerWidget * self,
                             float         zoom_level);

void
ruler_widget_refresh (RulerWidget * self);

#endif
