/*
 * inc/gui/widgets/editor_notebook.h - Editor notebook (bot of arranger)
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

#ifndef __GUI_WIDGETS_PIANO_ROLL_H__
#define __GUI_WIDGETS_PIANO_ROLL_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_WIDGET_TYPE \
  (piano_roll_widget_get_type ())
G_DECLARE_FINAL_TYPE (PianoRollWidget,
                      piano_roll_widget,
                      Z,
                      PIANO_ROLL_WIDGET,
                      GtkBox)

#define PIANO_ROLL MW_BOT_DOCK_EDGE->piano_roll

typedef struct _PianoRollLabelsWidget PianoRollLabelsWidget;
typedef struct _PianoRollNotesWidget PianoRollNotesWidget;
typedef struct _MidiArrangerWidget MidiArrangerWidget;
typedef struct _MidiRulerWidget MidiRulerWidget;
typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct _MidiModifierArrangerWidget
  MidiModifierArrangerWidget;

typedef struct _PianoRollWidget
{
  GtkBox                   parent_instance;
  ColorAreaWidget          * color_bar;
  GtkToolbar               * midi_bot_toolbar;
  GtkLabel                 * midi_name_label;
  GtkBox                   * midi_controls_above_notes_box;
  GtkScrolledWindow        * midi_ruler_scroll;
  GtkViewport              * midi_ruler_viewport;
  MidiRulerWidget *        ruler;
  GtkScrolledWindow        * piano_roll_labels_scroll;
  GtkViewport              * piano_roll_labels_viewport;
  PianoRollLabelsWidget    * piano_roll_labels;
  GtkScrolledWindow        * piano_roll_notes_scroll;
  GtkViewport              * piano_roll_notes_viewport;
  PianoRollNotesWidget     * piano_roll_notes;
  GtkBox                   * midi_arranger_box; ///< piano roll
  GtkScrolledWindow        * arranger_scroll;
  GtkViewport              * arranger_viewport;
  MidiArrangerWidget *     arranger;
  GtkScrolledWindow *      modifier_arranger_scroll;
  GtkViewport *            modifier_arranger_viewport;
  MidiModifierArrangerWidget * modifier_arranger;
  PianoRoll *              piano_roll; ///< pointer to backend struct
} MidiEditorWidget;

void
piano_roll_widget_setup (
  PianoRollWidget * self,
  PianoRoll *       pr);

#endif
