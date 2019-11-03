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

#ifndef __Z_TOOLKIT_ZTK_APP_H__
#define __Z_TOOLKIT_ZTK_APP_H__

#include "ztoolkit/ztk_theme.h"

#include <cairo.h>
#include <pugl/pugl.h>

typedef struct ZtkWidget ZtkWidget;

typedef struct ZtkApp
{
  PuglWorld *      world;
  PuglView *       view;

  char *           title;

  int              width;
  int              height;

  ZtkWidget **     widgets;
  int              num_widgets;
  int              widgets_size;

  /** Whether a mouse button is pressed */
  int              pressing;

  /** Start coordinates for events. */
  double           start_press_x;
  double           start_press_y;

  /** Previous coordinates for events. */
  double           prev_press_x;
  double           prev_press_y;

  /** Current coordinates for events. */
  double           offset_press_x;
  double           offset_press_y;

  //void *           parent;

  ZtkTheme         theme;
} ZtkApp;

/**
 * Creates a new ZtkApp.
 *
 * @param parent Parent window, if any.
 */
ZtkApp *
ztk_app_new (
  PuglWorld *      world,
  const char *     title,
  PuglNativeWindow parent,
  int              width,
  int              height);

/**
 * Adds a widget with the given Z axis.
 */
void
ztk_app_add_widget (
  ZtkApp *    self,
  ZtkWidget * widget,
  int         z);

/**
 * Removes the given widget from the app.
 */
void
ztk_app_remove_widget (
  ZtkApp *    self,
  ZtkWidget * widget);

/**
 * Draws each widget.
 */
void
ztk_app_draw (
  ZtkApp *  self,
  cairo_t * cr);

void
ztk_app_idle (
  ZtkApp * self);

/**
 * Shows the window.
 */
void
ztk_app_show_window (
  ZtkApp * self);

/**
 * Hides the window.
 */
void
ztk_app_hide_window (
  ZtkApp * self);

/**
 * Frees the app.
 */
void
ztk_app_free (
  ZtkApp * self);

#endif
