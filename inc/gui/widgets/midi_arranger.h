/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_MIDI_ARRANGER_H__
#define __GUI_WIDGETS_MIDI_ARRANGER_H__

#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"
#include "audio/position.h"

#include <gtk/gtk.h>

#define MIDI_ARRANGER_WIDGET_TYPE \
  (midi_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiArrangerWidget,
                      midi_arranger_widget,
                      Z,
                      MIDI_ARRANGER_WIDGET,
                      ArrangerWidget)

#define MIDI_ARRANGER MW_PIANO_ROLL->arranger

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct _MidiNoteWidget MidiNoteWidget;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct Region MidiRegion;
typedef struct Channel Channel;

typedef struct _MidiArrangerWidget
{
  ArrangerWidget           parent_instance;

  /**
   * Start MIDI note acting on. This is the note
   * that was clicked, even though there could be
   * more selected.
   */
  MidiNote *               start_midi_note;

  /**
   * Clone of the start MidiNote to reference
   * parameters during actions.
   */
  //MidiNote *               start_midi_note_clone;

  /** The note currently hovering over */
  int                      hovered_note;
} MidiArrangerWidget;

ARRANGER_W_DECLARE_FUNCS (
  Midi, midi);
ARRANGER_W_DECLARE_CHILD_OBJ_FUNCS (
  Midi, midi, MidiNote, note);

int
midi_arranger_widget_get_note_at_y (double y);

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 */
void
midi_arranger_widget_create_note (
  MidiArrangerWidget * self,
  Position * pos,
  int                  note,
  MidiRegion * region);

/**
 * Called during drag_update in the parent when
 * resizing the selection. It sets the start
 * Position of the selected MidiNote's.
 *
 * @param pos Absolute position in the arrranger.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
midi_arranger_widget_snap_midi_notes_l (
  MidiArrangerWidget *self,
  Position *          pos,
  int                 dry_run);

/**
 * Called during drag_update in the parent when
 * resizing the selection. It sets the end
 * Position of the selected MidiNote's.
 *
 * @param pos Absolute position in the arrranger.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
midi_arranger_widget_snap_midi_notes_r (
  MidiArrangerWidget *self,
  Position *          pos,
  int                 dry_run);

/**
 * if midi_notes are within the min border distance grid
 * will auto scroll
 */

void
midi_arranger_widget_auto_scroll(
	MidiArrangerWidget * self,
  GtkScrolledWindow *  scrolled_window,
  int                  transient);

#endif
