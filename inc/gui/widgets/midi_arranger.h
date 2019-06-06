/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
   * Start MIDI note acting on. This is the note that was
   * clicked, even though there could be more selected.
   */
  MidiNote *               start_midi_note;

  /** The note currently hovering over */
  int                      hovered_note;
} MidiArrangerWidget;

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

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
midi_arranger_widget_get_cursor (
  MidiArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool);

int
midi_arranger_widget_get_note_at_y (double y);

MidiNoteWidget *
midi_arranger_widget_get_hit_midi_note (MidiArrangerWidget *  self,
                                 double            x,
                                 double            y);

/**
 * Sets transient notes and actual notes
 * visibility based on the current action.
 */
void
midi_arranger_widget_update_visibility (
  MidiArrangerWidget * self);

void
midi_arranger_widget_select_all (
  MidiArrangerWidget *  self,
  int               select);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
midi_arranger_widget_show_context_menu (
  MidiArrangerWidget * self,
  gdouble              x,
  gdouble              y);

/**
 * Called on drag begin in parent when a note is hit and the
 * number of clicks is 1.
 */
void
midi_arranger_widget_on_drag_begin_note_hit (
  MidiArrangerWidget * self,
  double                   start_x,
  MidiNoteWidget *     midi_note_widget);

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

void
midi_arranger_widget_refresh_size (
  MidiArrangerWidget * self);

/**
 * Sets up the widget.
 */
void
midi_arranger_widget_setup (
  MidiArrangerWidget * self);

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
void
midi_arranger_widget_select (
  MidiArrangerWidget * self,
  double               offset_x,
  double               offset_y,
  int                  delete);

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
midi_arranger_widget_move_items_x (
  MidiArrangerWidget * self,
  long                 ticks_diff);

/**
 * Called when moving midi notes in drag update in arranger
 * widget for moving up/down (changing note).
 */
void
midi_arranger_widget_move_items_y (
  MidiArrangerWidget *self,
  double              offset_y);

/**
 * if midi_notes are within the min border distance grid
 * will auto scroll
 */

void
midi_arranger_widget_auto_scroll(
	MidiArrangerWidget * self,
  GtkScrolledWindow *  scrolled_window,
  int                  transient);

/**
 * Called on drag end.
 *
 * Sets default cursors back and sets the start midi note
 * to NULL if necessary.
 */
void
midi_arranger_widget_on_drag_end (
  MidiArrangerWidget * self);

/**
 * Readd children.
 */
void
midi_arranger_widget_refresh_children (
  MidiArrangerWidget * self);

#endif
