/*
 * Copyright (C) 2019-2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Automation mode.
 */

#ifndef __GUI_WIDGETS_AUTOMATION_MODE_H__
#define __GUI_WIDGETS_AUTOMATION_MODE_H__

#include "audio/automation_track.h"
#include "gui/widgets/custom_button.h"

#include <gtk/gtk.h>

typedef struct AutomationTrack AutomationTrack;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define AUTOMATION_MODE_HPADDING 3
#define AUTOMATION_MODE_HSEPARATOR_SIZE 1

/**
 * Custom button group to be drawn inside drawing
 * areas.
 */
typedef struct AutomationModeWidget
{
  /** X/y relative to parent drawing area. */
  double x;
  double y;

  /** Total width/height. */
  int width;
  int height;

  /** Width of each button, including padding. */
  //int                widths[NUM_AUTOMATION_MODES];

  int text_widths[NUM_AUTOMATION_MODES];
  int text_heights[NUM_AUTOMATION_MODES];
  int max_text_height;

  int has_hit_mode;

  /** Currently hit mode. */
  AutomationMode hit_mode;

  /** Default color. */
  GdkRGBA def_color;

  /** Hovered color. */
  GdkRGBA hovered_color;

  /** Toggled color. */
  GdkRGBA toggled_colors[NUM_AUTOMATION_MODES];

  /** Held color (used after clicking and before
   * releasing). */
  GdkRGBA held_colors[NUM_AUTOMATION_MODES];

  /** Aspect ratio for the rounded rectangle. */
  double aspect;

  /** Corner curvature radius for the rounded
   * rectangle. */
  double corner_radius;

  /** Used to update caches if state changed. */
  CustomButtonWidgetState
    last_states[NUM_AUTOMATION_MODES];

  /** Used during drawing. */
  CustomButtonWidgetState
    current_states[NUM_AUTOMATION_MODES];

  /** Used during transitions. */
  GdkRGBA last_colors[NUM_AUTOMATION_MODES];

  /** Cache layout for drawing text. */
  PangoLayout * layout;

  /** Owner. */
  AutomationTrack * owner;

  /** Frames left for a transition in color. */
  int transition_frames;

} AutomationModeWidget;

/**
 * Creates a new track widget from the given track.
 */
AutomationModeWidget *
automation_mode_widget_new (
  int               height,
  PangoLayout *     layout,
  AutomationTrack * owner);

void
automation_mode_widget_init (
  AutomationModeWidget * self);

void
automation_mode_widget_draw (
  AutomationModeWidget *  self,
  GtkSnapshot *           snapshot,
  double                  x,
  double                  y,
  double                  x_cursor,
  CustomButtonWidgetState state);

void
automation_mode_widget_free (
  AutomationModeWidget * self);

/**
 * @}
 */

#endif
