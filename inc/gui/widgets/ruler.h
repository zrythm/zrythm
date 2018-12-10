/*
 * inc/gui/widgets/ruler.h - Ruler
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#define RULER_WIDGET_TYPE                  (ruler_widget_get_type ())
G_DECLARE_FINAL_TYPE (RulerWidget,
                      ruler_widget,
                      RULER,
                      WIDGET,
                      GtkDrawingArea)

#define MW_RULER MW_CENTER_DOCK->ruler

/**
 * pixels to draw between each beat,
 * before being adjusted for zoom.
 * used by the ruler and timeline
 */
#define DEFAULT_PX_PER_TICK 0.03f

#define SPACE_BEFORE_START 10 /* pixels to put before 1st bar */

typedef struct _RulerWidget
{
  GtkDrawingArea           parent_instance;
  int                      px_per_beat;
  int                      px_per_bar;
  int                      px_per_quarter_beat;
  float                    px_per_tick;
  int                      total_px;
  double                   start_x; ///< for dragging
  GtkGestureDrag           * drag;
  GtkGestureMultiPress     * multipress;
} RulerWidget;

/**
 * Creates a ruler widget using the given ruler data.
 */
RulerWidget *
ruler_widget_new ();

/**
 * Turns px position into position.
 */
void
ruler_widget_px_to_pos (
           Position * pos, ///< position to fill in
           int      px); ///< pixels

int
ruler_widget_pos_to_px (Position * pos);

#endif
