/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Fader widget.
 */

#ifndef __GUI_WIDGETS_FADER_H__
#define __GUI_WIDGETS_FADER_H__

#include <gtk/gtk.h>

#define FADER_WIDGET_TYPE \
  (fader_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderWidget,
  fader_widget,
  Z,
  FADER_WIDGET,
  GtkWidget)

typedef struct Fader Fader;

/**
 * @addtogroup widgets
 * @{
 */

/**
 * A FaderWidget reflects a Fader object's state.
 */
typedef struct _FaderWidget
{
  GtkWidget        parent_instance;
  GtkGestureDrag * drag;
  Fader *          fader;
  double           last_x;
  double           last_y;
  bool             hover;
  bool             dragging;
  //GtkWindow *        tooltip_win;
  //GtkLabel *         tooltip_label;
  GdkRGBA start_color;
  GdkRGBA end_color;

  /** Value at start. */
  float amp_at_start;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} FaderWidget;

/**
 * Creates a new Fader widget and binds it to the
 * given Fader.
 */
void
fader_widget_setup (
  FaderWidget * self,
  Fader *       fader,
  int           width,
  int           height);

/**
 * @}
 */

#endif
