// clang-format off
// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * @file
 *
 * API for selections in the piano roll.
 */

#ifndef __GUI_BACKEND_MA_SELECTIONS_H__
#define __GUI_BACKEND_MA_SELECTIONS_H__

#include "dsp/midi_note.h"
#include "gui/backend/arranger_selections.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define MA_SELECTIONS (PROJECT->midi_arranger_selections)

/**
 * A collection of selected MidiNote's.
 *
 * Used for copying, undoing, etc.
 *
 * @extends ArrangerSelections
 */
typedef struct MidiArrangerSelections
{
  /** Base struct. */
  ArrangerSelections base;

  /** Selected notes. */
  MidiNote ** midi_notes;
  int         num_midi_notes;
  size_t      midi_notes_size;

} MidiArrangerSelections;

/**
 * @memberof MidiArrangerSelections
 */
MidiNote *
midi_arranger_selections_get_highest_note (MidiArrangerSelections * mas);

MidiNote *
midi_arranger_selections_get_lowest_note (MidiArrangerSelections * mas);

/**
 * Sets the listen status of notes on and off based
 * on changes in the previous selections and the
 * current selections.
 */
void
midi_arranger_selections_unlisten_note_diff (
  MidiArrangerSelections * prev,
  MidiArrangerSelections * mas);

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region Region to paste to.
 */
int
midi_arranger_selections_can_be_pasted (
  MidiArrangerSelections * ts,
  Position *               pos,
  Region *                 region);

NONNULL void
midi_arranger_selections_sort_by_pitch (MidiArrangerSelections * self, bool desc);

/**
 * @}
 */

#endif
