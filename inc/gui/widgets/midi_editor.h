/*
 * inc/gui/widgets/editor_notebook.h - Editor notebook (bot of arranger)
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

#ifndef __GUI_WIDGETS_MIDI_EDITOR_H__
#define __GUI_WIDGETS_MIDI_EDITOR_H__

#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

#define MIDI_EDITOR_WIDGET_TYPE                  (midi_editor_widget_get_type ())
#define MIDI_EDITOR_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIDI_EDITOR_WIDGET_TYPE, MidiEditorWidget))
#define MIDI_EDITOR_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), MIDI_EDITOR_WIDGET, MidiEditorWidgetClass))
#define IS_MIDI_EDITOR_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDI_EDITOR_WIDGET_TYPE))
#define IS_MIDI_EDITOR_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), MIDI_EDITOR_WIDGET_TYPE))
#define MIDI_EDITOR_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), MIDI_EDITOR_WIDGET_TYPE, MidiEditorWidgetClass))

#define MIDI_EDITOR MAIN_WINDOW->midi_editor

typedef struct PianoRollLabelsWidget PianoRollLabelsWidget;
typedef struct PianoRollNotesWidget PianoRollNotesWidget;
typedef struct MidiArranger MidiArranger;

typedef struct MidiEditorWidget
{
  GtkBox                   parent_instance;
  GtkBox                   * midi_track_color_box;
  ColorAreaWidget          * midi_track_color;
  GtkToolbar               * midi_bot_toolbar;
  GtkLabel                 * midi_name_label;
  GtkBox                   * midi_controls_above_notes_box;
  GtkBox                   * midi_ruler_box;
  GtkScrolledWindow        * midi_ruler_scroll;
  GtkViewport              * midi_ruler_viewport;
  RulerWidget              * midi_ruler;
  GtkBox                   * midi_notes_labels_box; ///< shows note labels C, C#, etc.
  GtkScrolledWindow        * piano_roll_labels_scroll;
  GtkViewport              * piano_roll_labels_viewport;
  PianoRollLabelsWidget    * piano_roll_labels;
  GtkBox                   * midi_notes_draw_box; ///< shows piano roll
  GtkScrolledWindow        * piano_roll_notes_scroll;
  GtkViewport              * piano_roll_notes_viewport;
  PianoRollNotesWidget     * piano_roll_notes;
  GtkBox                   * midi_arranger_box; ///< piano roll
  GtkScrolledWindow        * piano_roll_arranger_scroll;
  GtkViewport              * piano_roll_arranger_viewport;
  MidiArrangerWidget       * midi_arranger;
} MidiEditorWidget;

typedef struct MidiEditorWidgetClass
{
  GtkBoxClass       parent_class;
} MidiEditorWidgetClass;

MidiEditorWidget *
midi_editor_widget_new ();

/**
 * Sets up the MIDI editor for the given region.
 */
void
midi_editor_set_channel (Channel * channel);

#endif
