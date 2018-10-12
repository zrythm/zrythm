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
#define PIANO_ROLL_NOTES_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PIANO_ROLL_NOTES_WIDGET_TYPE, PianoRollNotesWidget))
#define PIANO_ROLL_NOTES_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), PIANO_ROLL_NOTES_WIDGET, PianoRollNotesWidgetClass))
#define IS_PIANO_ROLL_NOTES_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PIANO_ROLL_NOTES_WIDGET_TYPE))
#define IS_PIANO_ROLL_NOTES_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), PIANO_ROLL_NOTES_WIDGET_TYPE))
#define PIANO_ROLL_NOTES_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), PIANO_ROLL_NOTES_WIDGET_TYPE, PianoRollNotesWidgetClass))

#define DEFAULT_PX_PER_NOTE 8

typedef struct PianoRollNotesWidget
{
  GtkDrawingArea          parent_instance;
  GtkGestureDrag          * drag;
  GtkGestureMultiPress    * multipress;
  int                     start_y; ///< for dragging
  int                     note; ///< current note
} PianoRollNotesWidget;

typedef struct PianoRollNotesWidgetClass
{
  GtkDrawingAreaClass       parent_class;
} PianoRollNotesWidgetClass;

/**
 * Creates a piano_roll_notes.
 */
PianoRollNotesWidget *
piano_roll_notes_widget_new ();

#endif



