/*
 * inc/gui/widgets/arranger.h - MIDI arranger widget
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

#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll_page.h"
#include "audio/position.h"

#include <gtk/gtk.h>

#define MIDI_ARRANGER_WIDGET_TYPE (midi_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiArrangerWidget,
                      midi_arranger_widget,
                      MIDI_ARRANGER,
                      WIDGET,
                      ArrangerWidget)

#define MIDI_ARRANGER PIANO_ROLL_PAGE->midi_arranger

typedef struct ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct MidiNoteWidget MidiNoteWidget;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct MidiRegion MidiRegion;

typedef struct _MidiArrangerWidget
{
  ArrangerWidget           parent_instance;

  /**
   * MIDI notes acting on.
   */
  MidiNote *               midi_notes[600];
  int                      num_midi_notes;

  /**
   * Start MIDI note acting on. This is the note that was
   * clicked, even though there could be more selected.
   */
  MidiNote *               start_midi_note;

  /** Temporary start positions, set on drag_begin and
   * used in drag_update to move the objects accordingly
   */
  Position                 midi_note_start_poses[600];

  /**
   * Channel currently attached to MIDI arranger.
   */
  Channel *                channel;
} MidiArrangerWidget;

/**
 * Creates a timeline widget using the given timeline data.
 */
MidiArrangerWidget *
midi_arranger_widget_new (SnapGrid * snap_grid);

/**
 * Sets up the MIDI editor for the given region.
 */
void
midi_arranger_widget_set_channel (
  MidiArrangerWidget * arranger,
  Channel *            channel);

void
midi_arranger_widget_toggle_select_midi_note (
  MidiArrangerWidget * self,
  MidiNote *           midi_note,
  int                  append);

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
midi_arranger_widget_set_allocation (
  MidiArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation);

int
midi_arranger_widget_get_note_at_y (double y);

MidiNoteWidget *
midi_arranger_widget_get_hit_midi_note (MidiArrangerWidget *  self,
                                 double            x,
                                 double            y);

void
midi_arranger_widget_update_inspector (MidiArrangerWidget *self);

void
midi_arranger_widget_select_all (MidiArrangerWidget *  self,
                                 int               select);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
midi_arranger_widget_show_context_menu (MidiArrangerWidget * self);

/**
 * Called on drag begin in parent when a note is hit and the
 * number of clicks is 1.
 */
void
midi_arranger_widget_on_drag_begin_note_hit (
  MidiArrangerWidget * self,
  MidiNoteWidget *     midi_note_widget);

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 */
void
midi_arranger_widget_on_drag_begin_create_note (
  MidiArrangerWidget * self,
  Position * pos,
  int                  note,
  MidiRegion * region);

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 */
void
midi_arranger_widget_find_and_select_midi_notes (
  MidiArrangerWidget * self,
  double               offset_x,
  double               offset_y);

/**
 * Called during drag_update in parent when resizing the
 * selection. It sets the start pos of the selected MIDI
 * notes.
 */
void
midi_arranger_widget_snap_midi_notes_l (
  MidiArrangerWidget *self,
  Position *          pos);

/**
 * Called during drag_update in parent when resizing the
 * selection. It sets the end pos of the selected MIDI
 * notes.
 */
void
midi_arranger_widget_snap_midi_notes_r (
  MidiArrangerWidget *self,
  Position *          pos);

/**
 * Called when moving midi notes in drag update in arranger
 * widget.
 */
void
midi_arranger_widget_move_midi_notes_x (
  MidiArrangerWidget *self,
  Position *          pos);

/**
 * Called when moving midi notes in drag update in arranger
 * widget for moving up/down (changing note).
 */
void
midi_arranger_widget_move_midi_notes_y (
  MidiArrangerWidget *self,
  double              offset_y);

/**
 * Called on drag end.
 *
 * Sets default cursors back and sets the start midi note
 * to NULL if necessary.
 */
void
midi_arranger_widget_on_drag_end (
  MidiArrangerWidget * self);

GType midi_arranger_widget_get_type(void);

#endif
