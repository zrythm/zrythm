/*
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file */

#ifndef __GUI_WIDGETS_PIANO_ROLL_NOTES_H__
#define __GUI_WIDGETS_PIANO_ROLL_NOTES_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_NOTES_WIDGET_TYPE                  (piano_roll_notes_widget_get_type ())
G_DECLARE_FINAL_TYPE (PianoRollNotesWidget,
                      piano_roll_notes_widget,
                      PIANO_ROLL_NOTES,
                      WIDGET,
                      GtkDrawingArea)

#define DEFAULT_PX_PER_NOTE 8

typedef struct _PianoRollNotesWidget
{
  GtkDrawingArea          parent_instance;
  GtkGestureDrag          * drag;
  GtkGestureMultiPress    * multipress;
  int                     start_y; ///< for dragging
  int                     note; ///< current note
} PianoRollNotesWidget;

#endif
