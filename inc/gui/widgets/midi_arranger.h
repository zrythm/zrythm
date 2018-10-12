/*
 * inc/gui/widgets/midi_arranger.h - MIDI arranger widget
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

#ifndef __GUI_WIDGETS_MIDI_ARRANGER_H__
#define __GUI_WIDGETS_MIDI_ARRANGER_H__

#include <gtk/gtk.h>

#define MIDI_ARRANGER_WIDGET_TYPE                  (midi_arranger_widget_get_type ())
#define MIDI_ARRANGER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIDI_ARRANGER_WIDGET_TYPE, MidiArrangerWidget))
#define MIDI_ARRANGER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), MIDI_ARRANGER_WIDGET, MidiArrangerWidgetClass))
#define IS_MIDI_ARRANGER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDI_ARRANGER_WIDGET_TYPE))
#define IS_MIDI_ARRANGER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), MIDI_ARRANGER_WIDGET_TYPE))
#define MIDI_ARRANGER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), MIDI_ARRANGER_WIDGET_TYPE, MidiArrangerWidgetClass))

typedef struct MidiArrangerBgWidget MidiArrangerBgWidget;
typedef struct MidiNote MidiNote;

typedef enum MidiArrangerWidgetAction
{
  MAA_NONE,
  MAA_RESIZING_NOTE_L,
  MAA_RESIZING_NOTE_R,
  MAA_MOVING_NOTE,
  MAA_SELECTING_AREA
  //MAA_CREATING_NOTE
} MidiArrangerWidgetAction;

/**
 * TODO rename to arranger
 */
typedef struct MidiArrangerWidget
{
  GtkOverlay               parent_instance;
  MidiArrangerBgWidget     * bg;
  GtkDrawingArea           drawing_area;
  GtkGestureDrag           * drag;
  GtkGestureMultiPress     * multipress;
  double                   last_y;
  double                   last_offset_x;  ///< for dragging regions
  MidiArrangerWidgetAction     action;
  MidiNote                 * midi_note;  ///< MIDI note doing action upon, if any
  double                   start_x; ///< for dragging
  int                      n_press; ///< for multipress
} MidiArrangerWidget;

typedef struct MidiArrangerWidgetClass
{
  GtkOverlayClass       parent_class;
} MidiArrangerWidgetClass;

/**
 * Creates a timeline widget using the given timeline data.
 */
MidiArrangerWidget *
midi_arranger_widget_new ();

/**
 * Sets up the MIDI editor for the given region.
 */
void
midi_arranger_widget_set_channel (Channel * channel);

#endif


