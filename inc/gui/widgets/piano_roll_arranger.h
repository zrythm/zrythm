/*
 * inc/gui/widgets/piano_roll_arranger.h - PianoRollArranger
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_ARRANGER_H__
#define __GUI_WIDGETS_PIANO_ROLL_ARRANGER_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_ARRANGER_WIDGET_TYPE                  (piano_roll_arranger_widget_get_type ())
#define PIANO_ROLL_ARRANGER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PIANO_ROLL_ARRANGER_WIDGET_TYPE, PianoRollArrangerWidget))
#define PIANO_ROLL_ARRANGER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), PIANO_ROLL_ARRANGER_WIDGET, PianoRollArrangerWidgetClass))
#define IS_PIANO_ROLL_ARRANGER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PIANO_ROLL_ARRANGER_WIDGET_TYPE))
#define IS_PIANO_ROLL_ARRANGER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), PIANO_ROLL_ARRANGER_WIDGET_TYPE))
#define PIANO_ROLL_ARRANGER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), PIANO_ROLL_ARRANGER_WIDGET_TYPE, PianoRollArrangerWidgetClass))

typedef struct PianoRollArrangerBgWidget PianoRollArrangerBgWidget;

typedef enum PianoRollArrangerWidgetAction
{
  PIANO_ROLL_ARRANGER_WIDGET_ACTION_NONE,
  PIANO_ROLL_ARRANGER_WIDGET_ACTION_RESIZING_NOTE_L,
  PIANO_ROLL_ARRANGER_WIDGET_ACTION_RESIZING_NOTE_R,
  PIANO_ROLL_ARRANGER_WIDGET_ACTION_MOVING_NOTE,
  PIANO_ROLL_ARRANGER_WIDGET_ACTION_SELECTING_AREA
} PianoRollArrangerWidgetAction;

/**
 * TODO rename to arranger
 */
typedef struct PianoRollArrangerWidget
{
  GtkOverlay               parent_instance;
  //PianoRollArrangerBgWidget         * bg;
  GtkDrawingArea           drawing_area;
  GtkGestureDrag           * drag;
  GtkGestureMultiPress     * multipress;
  double                   last_y;
  double                   last_offset_x;  ///< for dragging regions
  PianoRollArrangerWidgetAction     action;
  //MidiNote                 * note;  ///< midi note doing action upon, if any
  double                   start_x; ///< for dragging
} PianoRollArrangerWidget;

typedef struct PianoRollArrangerWidgetClass
{
  GtkOverlayClass       parent_class;
} PianoRollArrangerWidgetClass;

/**
 * Creates a piano_roll_arranger widget using the given piano_roll_arranger data.
 */
PianoRollArrangerWidget *
piano_roll_arranger_widget_new ();

#endif


