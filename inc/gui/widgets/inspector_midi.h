/*
 * gui/widgets/inspector_midi.h - A inspector_midi widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_INSPECTOR_MIDI_H__
#define __GUI_WIDGETS_INSPECTOR_MIDI_H__

#include <gtk/gtk.h>

#define INSPECTOR_MIDI_WIDGET_TYPE (inspector_midi_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorMidiWidget,
  inspector_midi_widget,
  Z,
  INSPECTOR_MIDI_WIDGET,
  GtkGrid)

typedef struct MidiNote MidiNote;

typedef struct _InspectorMidiWidget
{
  GtkGrid           parent_instance;
  GtkLabel *        header;
  GtkBox *          position_box;
  GtkBox *          length_box;
  GtkColorButton *  color;
  GtkToggleButton * mute_toggle;
} InspectorMidiWidget;

void
inspector_midi_widget_show_midi (
  InspectorMidiWidget * self,
  MidiNote **           midis,
  int                   num_midis);

#endif
