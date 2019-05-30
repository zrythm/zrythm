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
 * Chord widget.
 */

#ifndef __GUI_WIDGETS_CHORD_H__
#define __GUI_WIDGETS_CHORD_H__

#include <gtk/gtk.h>

#define CHORD_WIDGET_TYPE \
  (chord_widget_get_type ())
G_DECLARE_FINAL_TYPE (ChordWidget,
                      chord_widget,
                      Z,
                      CHORD_WIDGET,
                      GtkDrawingArea);

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct ZChord ZChord;

/**
 * Widget for chords inside the ChordTrack.
 */
typedef struct _ChordWidget
{
  GtkDrawingArea           parent_instance;
  ZChord *                 chord;
} ChordWidget;

/**
 * Creates a chord widget.
 */
ChordWidget *
chord_widget_new (
  ZChord * chord);

void
chord_widget_select (
  ChordWidget * self,
  int            select);

/**
 * @}
 */

#endif
