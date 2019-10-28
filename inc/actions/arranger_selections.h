/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Action for a group of ArrangerObject's.
 */

#ifndef __UNDO_ARRANGER_SELECTIONS_ACTION_H__
#define __UNDO_ARRANGER_SELECTIONS_ACTION_H__

#include <stdint.h>

#include "actions/undoable_action.h"
#include "audio/position.h"

typedef struct ArrangerSelections ArrangerSelections;
typedef struct ArrangerObject ArrangerObject;
typedef struct Position Position;
typedef struct QuantizeOptions QuantizeOptions;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Type used when the action is a RESIZE action.
 */
typedef enum ArrangerSelectionsActionResizeType
{
  ARRANGER_SELECTIONS_ACTION_RESIZE_L,
  ARRANGER_SELECTIONS_ACTION_RESIZE_R,
  ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP,
  ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP,
} ArrangerSelectionsActionResizeType;

/**
 * Type used when the action is an EDIT action.
 */
typedef enum ArrangerSelectionsActionEditType
{
  /** Edit the name of the ArrangerObject's in the
   * selection. */
  ARRANGER_SELECTIONS_ACTION_EDIT_NAME,

  /**
   * Edit a Position of the ArrangerObject's in
   * the selection.
   *
   * This will just set all of the positions on the
   * object.
   */
  ARRANGER_SELECTIONS_ACTION_EDIT_POS,

  /**
   * Edit a primitive (int, etc) member of
   * ArrangerObject's in the selection.
   *
   * This will simply set all relevant primitive
   * values in an ArrangerObject when doing/undoing.
   */
  ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,

  /** For editing the MusicalScale inside
   * ScaleObject's. */
  ARRANGER_SELECTIONS_ACTION_EDIT_SCALE,

  /** For ramping MidiNote velocities or
   * AutomationPoint values.
   * (this is handled by EDIT_PRIMITIVE) */
  //ARRANGER_SELECTIONS_ACTION_EDIT_RAMP,
} ArrangerSelectionsActionEditType;

/**
 * The action.
 */
typedef struct ArrangerSelectionsAction
{
  UndoableAction       parent_instance;

  /** A clone of the ArrangerSelections. */
  ArrangerSelections * sel;

  /** A clone of the ArrangerSelections after the
   * change (used in the EDIT action. */
  ArrangerSelections * sel_after;

  /** Type of edit action, if an Edit action. */
  ArrangerSelectionsActionEditType edit_type;

  ArrangerSelectionsActionResizeType resize_type;

  /** Ticks diff. */
  long                 ticks;
  /** Tracks moved. */
  int                  delta_tracks;
  /** Lanes moved. */
  int                  delta_lanes;
  /** Chords moved (up/down in the Chord editor). */
  int                  delta_chords;
  /** Delta of MidiNote pitch. */
  int                  delta_pitch;
  /** Delta of MidiNote velocity. */
  int                  delta_vel;

  /** String, when changing a string. */
  char *               str;

  /** Position, when changing a Position. */
  Position             pos;

  /** Used when splitting - these are the split
   * ArrangerObject's. */
  ArrangerObject *     r1[800];
  ArrangerObject *     r2[800];

  /**
   * If this is 1, the first "do" call does
   * nothing in some cases.
   *
   * Set internally and either used or ignored.
   */
  int                  first_run;

  /** QuantizeOptions clone, if quantizing. */
  QuantizeOptions *    opts;

  /** ArrangerSelections clone with quantized
   * positions. */
  ArrangerSelections * quantized_sel;

  /** The original velocities when ramping. */
  uint8_t *            vel_before;

  /** The velocities changed to when ramping. */
  uint8_t *            vel_after;

  /**
   * The arranger object, if the action can only
   * affect a single object rather than selections,
   * like a region name change.
   */
  ArrangerObject *     obj;
} ArrangerSelectionsAction;

/**
 * Creates a new action for creating/deleting objects.
 *
 * @param create 1 to create 0 to delte.
 */
UndoableAction *
arranger_selections_action_new_create_or_delete (
  ArrangerSelections * sel,
  const int            create);

#define \
arranger_selections_action_new_create(sel) \
  arranger_selections_action_new_create_or_delete ( \
    (ArrangerSelections *) sel, 1)

#define \
arranger_selections_action_new_delete(sel) \
  arranger_selections_action_new_create_or_delete ( \
    (ArrangerSelections *) sel, 0)

/**
 * Creates a new action for moving or duplicating
 * objects.
 *
 * @param move 1 to move, 0 to duplicate.
 */
UndoableAction *
arranger_selections_action_new_move_or_duplicate (
  ArrangerSelections * sel,
  const int            move,
  const long           ticks,
  const int            delta_chords,
  const int            delta_pitch,
  const int            delta_tracks,
  const int            delta_lanes);

#define \
arranger_selections_action_new_move( \
  sel,ticks,chords,pitch,tracks,lanes) \
  arranger_selections_action_new_move_or_duplicate ( \
    (ArrangerSelections *) sel, 1, ticks, chords, \
    pitch, tracks, lanes)
#define \
arranger_selections_action_new_duplicate( \
  sel,ticks,chords,pitch,tracks,lanes) \
  arranger_selections_action_new_move_or_duplicate ( \
    (ArrangerSelections *) sel, 0, ticks, chords, \
    pitch, tracks, lanes)

#define \
arranger_selections_action_new_move_timeline( \
  sel,ticks,delta_tracks,delta_lanes) \
  arranger_selections_action_new_move ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes)
#define \
arranger_selections_action_new_duplicate_timeline( \
  sel,ticks,delta_tracks,delta_lanes) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes)

#define \
arranger_selections_action_new_move_midi( \
  sel,ticks,delta_pitch) \
  arranger_selections_action_new_move ( \
    sel, ticks, 0, delta_pitch, 0, 0)
#define \
arranger_selections_action_new_duplicate_midi( \
  sel,ticks,delta_pitch) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, 0, delta_pitch, 0, 0)

#define \
arranger_selections_action_new_move_chord( \
  sel,ticks,delta_chords) \
  arranger_selections_action_new_move ( \
    sel, ticks, delta_chords, 0, 0, 0)
#define \
arranger_selections_action_new_duplicate_chord( \
  sel,ticks,delta_chords) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, delta_chords, 0, 0, 0)

#define \
arranger_selections_action_new_move_automation( \
  sel,ticks) \
  arranger_selections_action_new_move ( \
    sel, ticks, 0, 0, 0, 0)
#define \
arranger_selections_action_new_duplicate_automation( \
  sel,ticks) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, 0, 0, 0, 0)

/**
 * Creates a new action for editing properties
 * of an object.
 *
 * @param sel_before The selections before the
 *   change.
 * @param sel_after The selections after the
 *   change.
 * @param type Indication of which field has changed.
 */
UndoableAction *
arranger_selections_action_new_edit (
  ArrangerSelections *             sel_before,
  ArrangerSelections *             sel_after,
  ArrangerSelectionsActionEditType type);

/**
 * Creates a new action for splitting
 * ArrangerObject's.
 *
 * @param pos Global position to split at.
 */
UndoableAction *
arranger_selections_action_new_split (
  ArrangerSelections * sel,
  const Position *     pos);

/**
 * Creates a new action for resizing
 * ArrangerObject's.
 *
 * @param ticks How many ticks to add to the resizing
 *   edge.
 */
UndoableAction *
arranger_selections_action_new_resize (
  ArrangerSelections *               sel,
  ArrangerSelectionsActionResizeType type,
  const long                         ticks);

/**
 * Creates a new action fro quantizing
 * ArrangerObject's.
 *
 * @param opts Quantize options.
 */
UndoableAction *
arranger_selections_action_new_quantize (
  ArrangerSelections * sel,
  QuantizeOptions *    opts);

int
arranger_selections_action_do (
  ArrangerSelectionsAction * self);

int
arranger_selections_action_undo (
  ArrangerSelectionsAction * self);

char *
arranger_selections_action_stringize (
  ArrangerSelectionsAction * self);

void
arranger_selections_action_free (
  ArrangerSelectionsAction * self);

/**
 * @}
 */

#endif
