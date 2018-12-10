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

#ifndef __GUI_WIDGETS_PIANO_ROLL_PAGE_H__
#define __GUI_WIDGETS_PIANO_ROLL_PAGE_H__

#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

#define PIANO_ROLL_PAGE_WIDGET_TYPE                  (piano_roll_page_widget_get_type ())
G_DECLARE_FINAL_TYPE (PianoRollPageWidget,
                      piano_roll_page,
                      PIANO_ROLL_PAGE,
                      WIDGET,
                      GtkBox)

#define PIANO_ROLL_PAGE MW_BOT_DOCK_EDGE->piano_roll_page

typedef struct PianoRollLabelsWidget PianoRollLabelsWidget;
typedef struct PianoRollNotesWidget PianoRollNotesWidget;
typedef struct _MidiArrangerWidget MidiArrangerWidget;
typedef struct _RulerWidget RulerWidget;

typedef struct _PianoRollPageWidget
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
  MidiArrangerWidget *     midi_arranger;
} MidiEditorWidget;

PianoRollPageWidget *
piano_roll_page_widget_new ();

#endif
