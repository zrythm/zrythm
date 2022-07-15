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
 * Custom button to be drawn inside drawing areas.
 */

#ifndef __GUI_WIDGETS_CUSTOM_BUTTON_H__
#define __GUI_WIDGETS_CUSTOM_BUTTON_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES 9

typedef enum CustomButtonWidgetState
{
  CUSTOM_BUTTON_WIDGET_STATE_NORMAL,
  CUSTOM_BUTTON_WIDGET_STATE_HOVERED,
  CUSTOM_BUTTON_WIDGET_STATE_ACTIVE,
  CUSTOM_BUTTON_WIDGET_STATE_TOGGLED,

  /** Only border is toggled. */
  CUSTOM_BUTTON_WIDGET_STATE_SEMI_TOGGLED,
} CustomButtonWidgetState;

typedef enum CustomButtonWidgetOwner
{
  CUSTOM_BUTTON_WIDGET_OWNER_TRACK,
  CUSTOM_BUTTON_WIDGET_OWNER_LANE,
  CUSTOM_BUTTON_WIDGET_OWNER_AT,
} CustomButtonWidgetOwner;

/**
 * Custom button to be drawn inside drawing areas.
 */
typedef struct CustomButtonWidget
{
  /** Function to call on press (after click and
   * release). */
  //void (*press_cb) (void *);

  /** Whether the button is a toggle. */
  //int          is_toggle;

  /** Default color. */
  GdkRGBA def_color;

  /** Hovered color. */
  GdkRGBA hovered_color;

  /** Toggled color. */
  GdkRGBA toggled_color;

  /** Held color (used after clicking and before
   * releasing). */
  GdkRGBA held_color;

  /** Name of the icon to show. */
  char icon_name[120];

  /** Size in pixels (width and height will be set
   * to this). */
  int size;

  /** if non-zero, the button has "size" height and
   * this width. */
  int width;

  /** Whether currently hovered. */
  //int         hovered;

  /** Whether currently held down. */
  //int         pressed;

  /** Aspect ratio for the rounded rectangle. */
  double aspect;

  /** Corner curvature radius for the rounded
   * rectangle. */
  double corner_radius;

  /** Object to pass to the callback. */
  //void *       obj;

  /** The icon surface. */
  GdkTexture * icon_texture;

  /** Used to update caches if state changed. */
  CustomButtonWidgetState last_state;

  /** Owner type. */
  CustomButtonWidgetOwner owner_type;

  /** Owner. */
  void * owner;

  /** Used during transitions. */
  GdkRGBA last_color;

  /**
   * Text, if any, to show after the icon.
   *
   * This will be ellipsized.
   */
  char * text;

  int text_height;

  /** Cache layout for drawing the text. */
  PangoLayout * layout;

  /** X/y relative to parent drawing area. */
  double x;
  double y;

  /**
   * The id of the button returned by a symap of its
   * icon name, for better performance vs comparing
   * strings.
   *
   * TODO
   */
  unsigned int button_id;

  /** Frames left for a transition in color. */
  int transition_frames;

} CustomButtonWidget;

/**
 * Creates a new track widget from the given track.
 */
CustomButtonWidget *
custom_button_widget_new (const char * icon_name, int size);

void
custom_button_widget_draw (
  CustomButtonWidget *    self,
  GtkSnapshot *           snapshot,
  double                  x,
  double                  y,
  CustomButtonWidgetState state);

/**
 * @param width Max width for the button to use.
 */
void
custom_button_widget_draw_with_text (
  CustomButtonWidget *    self,
  GtkSnapshot *           snapshot,
  double                  x,
  double                  y,
  double                  width,
  CustomButtonWidgetState state);

/**
 * Sets the text and layout to draw the text width.
 *
 * @param font_descr Font description to set the
 *   pango layout font to.
 */
void
custom_button_widget_set_text (
  CustomButtonWidget * self,
  PangoLayout *        layout,
  char *               text,
  const char *         font_descr);

void
custom_button_widget_free (CustomButtonWidget * self);

/**
 * @}
 */

#endif
