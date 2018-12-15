/*
 * inc/gui/widgets/midi_arranger_bg.h - MidiArranger background
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

#ifndef __GUI_WIDGETS_MIDI_ARRANGER_BG_H__
#define __GUI_WIDGETS_MIDI_ARRANGER_BG_H__

#include <gtk/gtk.h>

#define MIDI_ARRANGER_BG_WIDGET_TYPE                  (midi_arranger_bg_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiArrangerBgWidget,
                      midi_arranger_bg_widget,
                      MIDI_ARRANGER_BG,
                      WIDGET,
                      GtkDrawingArea);

typedef struct _MidiArrangerBgWidget
{
  GtkDrawingArea           parent_instance;
  int                      total_px;
  double                   start_x; ///< for dragging
  GtkGestureDrag           * drag;
  GtkGestureMultiPress     * multipress;
} MidiArrangerBgWidget;

/**
 * Creates a timeline widget using the given timeline data.
 */
MidiArrangerBgWidget *
midi_arranger_bg_widget_new ();

#endif



