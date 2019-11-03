/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Ztoolkit base widget.
 */

#ifndef __Z_TOOLKIT_ZTK_WIDGET_H__
#define __Z_TOOLKIT_ZTK_WIDGET_H__

#include <cairo.h>
#include <pugl/pugl.h>

typedef struct ZtkApp ZtkApp;

/**
 * @addtogroup ztoolkit
 *
 * @{
 */

/**
 * Flags to use in the event callbacks.
 */
typedef enum ZtkEventChain
{
  /** Propagate the event down to the next hit
   * widget. */
  ZTK_EVENT_CHAIN_CONTINUE,
  /** Don't propagate the event. */
  ZTK_EVENT_CHAIN_STOP,
} ZtkEventChain;

/**
 * State of the widget.
 */
typedef enum ZtkWidgetState
{
  /** Normal state. */
	ZTK_WIDGET_STATE_NORMAL = 1,

  ZTK_WIDGET_STATE_HOVERED = 1 << 1,
  ZTK_WIDGET_STATE_SELECTED = 1 << 2,
  ZTK_WIDGET_STATE_PRESSED = 1  << 3,
} ZtkWidgetState;

//typedef enum ZtkWidgetType
//{
  //ZTK_WIDGET_TYPE_KNOB,
  //ZTK_WIDGET_TYPE_CUSTOM,
//} ZtkWidgetType;
//

typedef struct ZtkWidget ZtkWidget;

/**
 * Base widget.
 */
typedef struct ZtkWidget
{
  /** Pointer back to app. */
  ZtkApp *          app;

  /** Rectangle. */
  PuglRect          rect;

  ZtkWidgetState    state;

  /**
   * The type this widget is.
   *
   * Custom widgets should be
   * ZTK_WIDGET_TYPE_CUSTOM + increasing numbers.
   */
  //ZtkWidgetType     type;

  /** Update callback (called right before drawing)
   * (required. */
  void (*update_cb) (ZtkWidget *);

  /** Draw callback (required). */
  void (*draw_cb) (ZtkWidget *, cairo_t *);

  /** Free callback (required). */
  void (*free_cb) (ZtkWidget *);

  /**
   * Button event callback (optional).
   *
   * Returns if the
   * next hit widget on the stack should receive
   * the event (1) or if the chain should stop here
   * (0).
   */
  int (*button_event_cb) (ZtkWidget *, const PuglEventButton *);

  /** Button event callback (optional). */
  int (*key_event_cb) (ZtkWidget *, const PuglEventKey *);

  /** Enter/leave event callback (optional). */
  int (*crossing_event_cb) (ZtkWidget *, const PuglEventCrossing *);

  /** Motion event callback (optional). */
  int (*motion_event_cb) (ZtkWidget *, const PuglEventMotion *);

  /** Scroll event callback (optional). */
  int (*scroll_event_cb) (ZtkWidget *, const PuglEventScroll *);

  /* note: Ignore these below for now. */

  /** Visible or not. */
  int               visible;

  /**
   * Z-axis.
   *
   * Widgets with lower z values will be drawn at
   * the bottom.
   */
  int               z;

  /** Set to 1 to redraw. */
  int               redraw;

  /** Last rectangle drawn in. */
  PuglRect          last_draw_rect;

  /** Cairo caches. */
  cairo_t *         cached_cr;
  cairo_surface_t * cached_surface;

} ZtkWidget;

/**
 * Inits a new ZWidget.
 *
 * To be called from inheriting structs.
 */
void
ztk_widget_init (
  ZtkWidget *       self,
  PuglRect *        rect,
  void (*update_cb) (ZtkWidget *),
  void (*draw_cb) (ZtkWidget *, cairo_t *),
  void (*free_cb) (ZtkWidget *));

/**
 * Returns if the widget is hit by the given
 * coordinates.
 */
int
ztk_widget_is_hit (
  ZtkWidget * self,
  double      x,
  double      y);

/**
 * Draws the widget.
 */
void
ztk_widget_draw (
  ZtkWidget * self,
  cairo_t *   cr);

/**
 * @}
 */

#endif
