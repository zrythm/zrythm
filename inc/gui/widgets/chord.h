/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Single chord pad.
 */

#ifndef __GUI_WIDGETS_CHORD_H__
#define __GUI_WIDGETS_CHORD_H__

#include <stdbool.h>

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CHORD_WIDGET_TYPE \
  (chord_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordWidget, chord_widget, Z, CHORD_WIDGET,
  GtkWidget)

/**
 * Single chord pad.
 */
typedef struct _ChordWidget
{
  GtkWidget     parent_instance;

  /** Main child. */
  GtkOverlay *  overlay;

  /** Button. */
  GtkButton *   btn;

  GtkGestureDrag * btn_drag;

  double        drag_start_x;
  double        drag_start_y;

  /** Whether the drag has started. */
  bool          drag_started;

  GtkBox *      btn_box;
  GtkButton *   edit_chord_btn;
  GtkButton *   invert_prev_btn;
  GtkButton *   invert_next_btn;

  /** Index of the chord in the chord track. */
  int           idx;
} ChordWidget;

/**
 * Creates a chord widget.
 */
ChordWidget *
chord_widget_new (void);

/**
 * Sets the chord index on the chord widget.
 */
void
chord_widget_refresh (
  ChordWidget * self,
  int           idx);

/**
 * @}
 */

#endif
