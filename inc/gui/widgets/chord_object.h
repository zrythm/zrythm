/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Widget for ChordObject.
 */

#ifndef __GUI_WIDGETS_CHORD_OBJECT_H__
#define __GUI_WIDGETS_CHORD_OBJECT_H__

#include <gtk/gtk.h>

#define CHORD_OBJECT_WIDGET_TYPE \
  (chord_object_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordObjectWidget,
  chord_object_widget,
  Z, CHORD_OBJECT_WIDGET,
  GtkBox);

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct ChordObjectObject ChordObjectObject;

/**
 * Widget for chords inside the ChordObjectTrack.
 */
typedef struct _ChordObjectWidget
{
  GtkBox           parent_instance;
  GtkDrawingArea * drawing_area;
  ChordObject *    chord;
} ChordObjectWidget;

/**
 * Creates a chord widget.
 */
ChordObjectWidget *
chord_object_widget_new (
  ChordObject * chord);

void
chord_object_widget_select (
  ChordObjectWidget * self,
  int            select);

/**
 * @}
 */

#endif
