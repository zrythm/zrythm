/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/position.h"
#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

typedef struct _ArrangerWidget ArrangerWidget;
typedef struct MidiNote        MidiNote;
typedef struct SnapGrid        SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct ZRegion         MidiRegion;
typedef struct Channel         Channel;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MIDI_ARRANGER \
  (MW_MIDI_EDITOR_SPACE->arranger)

/**
 * Returns the note value (0-127) at y.
 */
//int
//midi_arranger_widget_get_note_at_y (double y);

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 */
void
midi_arranger_widget_create_note (
  ArrangerWidget * self,
  Position *       pos,
  int              note,
  ZRegion *        region);

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
  ArrangerWidget * self,
  Position *       pos,
  bool             dry_run);

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
  ArrangerWidget * self,
  Position *       pos,
  bool             dry_run);

/**
 * Sets the currently hovered note and queues a
 * redraw if it changed.
 *
 * @param pitch The note pitch, or -1 for no note.
 */
void
midi_arranger_widget_set_hovered_note (
  ArrangerWidget * self,
  int              pitch);

/**
 * Resets the transient of each note in the
 * arranger.
 */
void
midi_arranger_widget_reset_transients (
  ArrangerWidget * self);

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
int
midi_arranger_calc_deltamax_for_note_movement (
  int y_delta);

/**
 * Listen to the currently selected notes.
 *
 * This function either turns on the notes if they
 * are not playing, changes the notes if the pitch
 * changed, or otherwise does nothing.
 *
 * @param listen Turn notes on if 1, or turn them
 *   off if 0.
 */
void
midi_arranger_listen_notes (
  ArrangerWidget * self,
  bool             listen);

void
midi_arranger_show_context_menu (
  ArrangerWidget * self,
  gdouble          x,
  gdouble          y);

/**
 * Handle ctrl+shift+scroll.
 */
void
midi_arranger_handle_vertical_zoom_scroll (
  ArrangerWidget *           self,
  GtkEventControllerScroll * scroll_controller,
  double                     dy);

/**
 * @}
 */

#endif
