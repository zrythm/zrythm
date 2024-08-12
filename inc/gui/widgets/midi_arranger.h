// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_MIDI_ARRANGER_H__
#define __GUI_WIDGETS_MIDI_ARRANGER_H__

#include "gui/widgets/arranger.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MIDI_ARRANGER (MW_MIDI_EDITOR_SPACE->arranger_wrapper->child)

/**
 * Called on drag begin in parent when background is double clicked (i.e., a
 * note is created).
 */
void
midi_arranger_widget_create_note (
  ArrangerWidget * self,
  const Position   pos,
  int              note,
  MidiRegion      &region);

/**
 * Called during drag_update in the parent when resizing the selection. It sets
 * the start Position of the selected MidiNote's.
 *
 * @param pos Absolute position in the arrranger.
 * @param dry_run Don't resize notes; just check if the resize is allowed (check
 * if invalid resizes will happen)
 *
 * @return Whether successful.
 */
bool
midi_arranger_widget_snap_midi_notes_l (
  ArrangerWidget * self,
  const Position   pos,
  bool             dry_run);

/**
 * Called during drag_update in the parent when resizing the selection. It sets
 * the end Position of the selected MidiNote's.
 *
 * @param pos Absolute position in the arrranger.
 * @parram dry_run Don't resize notes; just check if the resize is allowed
 * (check if invalid resizes will happen)
 *
 * @return Whether successful.
 */
bool
midi_arranger_widget_snap_midi_notes_r (
  ArrangerWidget * self,
  const Position   pos,
  bool             dry_run);

/**
 * Sets the currently hovered note and queues a
 * redraw if it changed.
 *
 * @param pitch The note pitch, or -1 for no note.
 */
void
midi_arranger_widget_set_hovered_note (ArrangerWidget * self, int pitch);

/**
 * Resets the transient of each note in the
 * arranger.
 */
void
midi_arranger_widget_reset_transients (ArrangerWidget * self);

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
int
midi_arranger_calc_deltamax_for_note_movement (int y_delta);

/**
 * To be used as a source function to unlisten notes.
 */
gboolean
midi_arranger_unlisten_notes_source_func (gpointer user_data);

/**
 * Listen to the currently selected notes.
 *
 * This function either turns on the notes if they are not
 * playing, changes the notes if the pitch changed, or
 * otherwise does nothing.
 *
 * @param listen Turn notes on if 1, or turn them off if 0.
 */
void
midi_arranger_listen_notes (ArrangerWidget * self, bool listen);

/**
 * Generate a context menu at x, y.
 *
 * @param menu A menu to append entries to (optional).
 *
 * @return The given updated menu or a new menu.
 */
GMenu *
midi_arranger_widget_gen_context_menu (ArrangerWidget * self, double x, double y);

void
midi_arranger_handle_vertical_zoom_action (ArrangerWidget * self, bool zoom_in);

/**
 * Handle ctrl+shift+scroll.
 */
void
midi_arranger_handle_vertical_zoom_scroll (
  ArrangerWidget *           self,
  GtkEventControllerScroll * scroll_controller,
  double                     dy);

void
midi_arranger_on_drag_end (ArrangerWidget * self);

/**
 * @}
 */

#endif
