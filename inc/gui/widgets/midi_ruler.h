/*
 * inc/gui/widgets/midi_ruler.h - MIDI ruler
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_MIDI_RULER_H__
#define __GUI_WIDGETS_MIDI_RULER_H__

#include "gui/widgets/ruler.h"

#include <gtk/gtk.h>

#define MIDI_RULER_WIDGET_TYPE \
  (midi_ruler_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiRulerWidget,
                      midi_ruler_widget,
                      Z,
                      MIDI_RULER_WIDGET,
                      RulerWidget)

#define MIDI_RULER PIANO_ROLL->ruler

typedef struct _MidiRulerWidget
{
  RulerWidget         parent_instance;

} MidiRulerWidget;

#endif
