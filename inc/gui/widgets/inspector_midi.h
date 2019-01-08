/*
 * gui/widgets/inspector_midi.h - A inspector_midi widget
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_INSPECTOR_MIDI_H__
#define __GUI_WIDGETS_INSPECTOR_MIDI_H__

#include <gtk/gtk.h>

#define INSPECTOR_MIDI_WIDGET_TYPE                  (inspector_midi_widget_get_type ())
#define INSPECTOR_MIDI_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), INSPECTOR_MIDI_WIDGET_TYPE, InspectorMidiWidget))
#define INSPECTOR_MIDI_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), INSPECTOR_MIDI_WIDGET, InspectorMidiWidgetClass))
#define IS_INSPECTOR_MIDI_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INSPECTOR_MIDI_WIDGET_TYPE))
#define IS_INSPECTOR_MIDI_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), INSPECTOR_MIDI_WIDGET_TYPE))
#define INSPECTOR_MIDI_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), INSPECTOR_MIDI_WIDGET_TYPE, InspectorMidiWidgetClass))

typedef struct MidiNote MidiNote;

typedef struct InspectorMidiWidget
{
  GtkGrid             parent_instance;
  GtkLabel *          header;
  GtkBox *            position_box;
  GtkBox *            length_box;
  GtkColorButton *    color;
  GtkToggleButton *   mute_toggle;
} InspectorMidiWidget;

typedef struct InspectorMidiWidgetClass
{
  GtkGridClass       parent_class;
} InspectorMidiWidgetClass;

/**
 * Creates the inspector_midi widget.
 *
 * Only once per project.
 */
InspectorMidiWidget *
inspector_midi_widget_new ();

void
inspector_midi_widget_show_midi(InspectorMidiWidget * self,
                                      MidiNote **               midis,
                                      int                     num_midis);

#endif



