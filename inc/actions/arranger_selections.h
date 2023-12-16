// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Action for a group of ArrangerObject's.
 */

#ifndef __UNDO_ARRANGER_SELECTIONS_ACTION_H__
#define __UNDO_ARRANGER_SELECTIONS_ACTION_H__

#include <stdbool.h>
#include <stdint.h>

#include "actions/undoable_action.h"
#include "dsp/audio_function.h"
#include "dsp/automation_function.h"
#include "dsp/midi_function.h"
#include "dsp/port_identifier.h"
#include "dsp/position.h"
#include "dsp/quantize_options.h"
#include "gui/backend/audio_selections.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/timeline_selections.h"

typedef struct ArrangerSelections ArrangerSelections;
typedef struct ArrangerObject     ArrangerObject;
typedef struct Position           Position;
typedef struct QuantizeOptions    QuantizeOptions;

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum ArrangerSelectionsActionType
{
  AS_ACTION_AUTOMATION_FILL,
  AS_ACTION_CREATE,
  AS_ACTION_DELETE,
  AS_ACTION_DUPLICATE,
  AS_ACTION_EDIT,
  AS_ACTION_LINK,
  AS_ACTION_MERGE,
  AS_ACTION_MOVE,
  AS_ACTION_QUANTIZE,
  AS_ACTION_RECORD,
  AS_ACTION_RESIZE,
  AS_ACTION_SPLIT,
} ArrangerSelectionsActionType;

static const cyaml_strval_t arranger_selections_action_type_strings[] = {
  {"Automation fill", AS_ACTION_AUTOMATION_FILL},
  { "Create",         AS_ACTION_CREATE         },
  { "Delete",         AS_ACTION_DELETE         },
  { "Duplicate",      AS_ACTION_DUPLICATE      },
  { "Edit",           AS_ACTION_EDIT           },
  { "Link",           AS_ACTION_LINK           },
  { "Merge",          AS_ACTION_MERGE          },
  { "Move",           AS_ACTION_MOVE           },
  { "Quantize",       AS_ACTION_QUANTIZE       },
  { "Record",         AS_ACTION_RECORD         },
  { "Resize",         AS_ACTION_RESIZE         },
  { "Split",          AS_ACTION_SPLIT          },
};

/**
 * Type used when the action is a RESIZE action.
 */
typedef enum ArrangerSelectionsActionResizeType
{
  ARRANGER_SELECTIONS_ACTION_RESIZE_L,
  ARRANGER_SELECTIONS_ACTION_RESIZE_R,
  ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP,
  ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP,
  ARRANGER_SELECTIONS_ACTION_RESIZE_L_FADE,
  ARRANGER_SELECTIONS_ACTION_RESIZE_R_FADE,
  ARRANGER_SELECTIONS_ACTION_STRETCH_L,
  ARRANGER_SELECTIONS_ACTION_STRETCH_R,
} ArrangerSelectionsActionResizeType;

static const cyaml_strval_t arranger_selections_action_resize_type_strings[] = {
  {"Resize L",         ARRANGER_SELECTIONS_ACTION_RESIZE_L     },
  { "Resize R",        ARRANGER_SELECTIONS_ACTION_RESIZE_R     },
  { "Resize L (loop)", ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP},
  { "Resize R (loop)", ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP},
  { "Resize L (fade)", ARRANGER_SELECTIONS_ACTION_RESIZE_L_FADE},
  { "Resize R (fade)", ARRANGER_SELECTIONS_ACTION_RESIZE_R_FADE},
  { "Stretch L",       ARRANGER_SELECTIONS_ACTION_STRETCH_L    },
  { "Stretch R",       ARRANGER_SELECTIONS_ACTION_STRETCH_R    },
};

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

  /** Editing fade positions or curve options. */
  ARRANGER_SELECTIONS_ACTION_EDIT_FADES,

  /** Change mute status. */
  ARRANGER_SELECTIONS_ACTION_EDIT_MUTE,

  /** For ramping MidiNote velocities or
   * AutomationPoint values.
   * (this is handled by EDIT_PRIMITIVE) */
  // ARRANGER_SELECTIONS_ACTION_EDIT_RAMP,

  /** MIDI function. */
  ARRANGER_SELECTIONS_ACTION_EDIT_EDITOR_FUNCTION,
} ArrangerSelectionsActionEditType;

static const cyaml_strval_t arranger_selections_action_edit_type_strings[] = {
  {"Name",             ARRANGER_SELECTIONS_ACTION_EDIT_NAME           },
  { "Pos",             ARRANGER_SELECTIONS_ACTION_EDIT_POS            },
  { "Primitive",       ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE      },
  { "Scale",           ARRANGER_SELECTIONS_ACTION_EDIT_SCALE          },
  { "Fades",           ARRANGER_SELECTIONS_ACTION_EDIT_FADES          },
  { "Mute",            ARRANGER_SELECTIONS_ACTION_EDIT_MUTE           },
  { "Editor function", ARRANGER_SELECTIONS_ACTION_EDIT_EDITOR_FUNCTION},
};

/**
 * The action.
 */
typedef struct ArrangerSelectionsAction
{
  UndoableAction parent_instance;

  /** Action type. */
  ArrangerSelectionsActionType type;

  /** A clone of the ArrangerSelections before the change. */
  ArrangerSelections * sel;

  /**
   * A clone of the ArrangerSelections after the change (used
   * in the EDIT action and quantize).
   */
  ArrangerSelections * sel_after;

  /** Type of edit action, if an Edit action. */
  ArrangerSelectionsActionEditType edit_type;

  ArrangerSelectionsActionResizeType resize_type;

  /** Ticks diff. */
  double ticks;
  /** Tracks moved. */
  int delta_tracks;
  /** Lanes moved. */
  int delta_lanes;
  /** Chords moved (up/down in the Chord editor). */
  int delta_chords;
  /** Delta of MidiNote pitch. */
  int delta_pitch;
  /** Delta of MidiNote velocity. */
  int delta_vel;
  /**
   * Difference in a normalized amount, such as
   * automation point normalized value.
   */
  double delta_normalized_amount;

  /** Target port (used to find corresponding automation track
   * when moving/copying automation regions to another
   * automation track/another track). */
  PortIdentifier * target_port;

  /** String, when changing a string. */
  char * str;

  /** Position, when changing a Position. */
  Position pos;

  /** Used when splitting - these are the split
   * ArrangerObject's. */
  ArrangerObject * r1[800];
  ArrangerObject * r2[800];

  /** Number of split objects inside r1 and r2
   * each. */
  int num_split_objs;

  /**
   * If this is 1, the first "do" call does
   * nothing in some cases.
   *
   * Set internally and either used or ignored.
   */
  int first_run;

  /** QuantizeOptions clone, if quantizing. */
  QuantizeOptions * opts;

  /* --- below for serialization only --- */
  ChordSelections *        chord_sel;
  ChordSelections *        chord_sel_after;
  TimelineSelections *     tl_sel;
  TimelineSelections *     tl_sel_after;
  MidiArrangerSelections * ma_sel;
  MidiArrangerSelections * ma_sel_after;
  AutomationSelections *   automation_sel;
  AutomationSelections *   automation_sel_after;
  AudioSelections *        audio_sel;
  AudioSelections *        audio_sel_after;

  /* arranger objects that can be split */
  ZRegion *  region_r1[800];
  ZRegion *  region_r2[800];
  MidiNote * mn_r1[800];
  MidiNote * mn_r2[800];

  /** Used for automation autofill action. */
  ZRegion * region_before;
  ZRegion * region_after;

} ArrangerSelectionsAction;

void
arranger_selections_action_init_loaded (ArrangerSelectionsAction * self);

/**
 * Creates a new action for creating/deleting
 * objects.
 *
 * @param create 1 to create 0 to delete.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_create_or_delete (
  ArrangerSelections * sel,
  const bool           create,
  GError **            error);

#define arranger_selections_action_new_create(sel, error) \
  arranger_selections_action_new_create_or_delete ( \
    (ArrangerSelections *) sel, true, error)

#define arranger_selections_action_new_delete(sel, error) \
  arranger_selections_action_new_create_or_delete ( \
    (ArrangerSelections *) sel, false, error)

WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_record (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const bool           already_recorded,
  GError **            error);

/**
 * Creates a new action for moving or duplicating
 * objects.
 *
 * @param move True to move, false to duplicate.
 * @param already_moved If this is true, the first
 *   DO will do nothing.
 * @param delta_normalized_amount Difference in a
 *   normalized amount, such as automation point
 *   normalized value.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_move_or_duplicate (
  ArrangerSelections *   sel,
  const bool             move,
  const double           ticks,
  const int              delta_chords,
  const int              delta_pitch,
  const int              delta_tracks,
  const int              delta_lanes,
  const double           delta_normalized_amount,
  const PortIdentifier * tgt_port_id,
  const bool             already_moved,
  GError **              error);

#define arranger_selections_action_new_move( \
  sel, ticks, chords, pitch, tracks, lanes, norm_amt, port_id, already_moved, \
  error) \
  arranger_selections_action_new_move_or_duplicate ( \
    (ArrangerSelections *) sel, 1, ticks, chords, pitch, tracks, lanes, \
    norm_amt, port_id, already_moved, error)
#define arranger_selections_action_new_duplicate( \
  sel, ticks, chords, pitch, tracks, lanes, norm_amt, port_id, already_moved, \
  error) \
  arranger_selections_action_new_move_or_duplicate ( \
    (ArrangerSelections *) sel, 0, ticks, chords, pitch, tracks, lanes, \
    norm_amt, port_id, already_moved, error)

#define arranger_selections_action_new_move_timeline( \
  sel, ticks, delta_tracks, delta_lanes, port_id, already_moved, error) \
  arranger_selections_action_new_move ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes, 0, port_id, already_moved, \
    error)
#define arranger_selections_action_new_duplicate_timeline( \
  sel, ticks, delta_tracks, delta_lanes, port_id, already_moved, error) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes, 0, port_id, already_moved, \
    error)

#define arranger_selections_action_new_move_midi( \
  sel, ticks, delta_pitch, already_moved, error) \
  arranger_selections_action_new_move ( \
    sel, ticks, 0, delta_pitch, 0, 0, 0, NULL, already_moved, error)
#define arranger_selections_action_new_duplicate_midi( \
  sel, ticks, delta_pitch, already_moved, error) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, 0, delta_pitch, 0, 0, 0, NULL, already_moved, error)
#define arranger_selections_action_new_move_chord( \
  sel, ticks, delta_chords, already_moved, error) \
  arranger_selections_action_new_move ( \
    sel, ticks, delta_chords, 0, 0, 0, 0, NULL, already_moved, error)
#define arranger_selections_action_new_duplicate_chord( \
  sel, ticks, delta_chords, already_moved, error) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, delta_chords, 0, 0, 0, 0, NULL, already_moved, error)

#define arranger_selections_action_new_move_automation( \
  sel, ticks, norm_amt, already_moved, error) \
  arranger_selections_action_new_move ( \
    sel, ticks, 0, 0, 0, 0, norm_amt, NULL, already_moved, error)
#define arranger_selections_action_new_duplicate_automation( \
  sel, ticks, norm_amt, already_moved, error) \
  arranger_selections_action_new_duplicate ( \
    sel, ticks, 0, 0, 0, 0, norm_amt, NULL, already_moved, error)

/**
 * Creates a new action for linking regions.
 *
 * @param already_moved If this is true, the first
 *   DO will do nothing.
 * @param sel_before Original selections.
 * @param sel_after Selections after duplication.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_link (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const double         ticks,
  const int            delta_tracks,
  const int            delta_lanes,
  const bool           already_moved,
  GError **            error);

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
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_edit (
  ArrangerSelections *             sel_before,
  ArrangerSelections *             sel_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error);

WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_edit_single_obj (
  const ArrangerObject *           obj_before,
  const ArrangerObject *           obj_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error);

/**
 * Wrapper over
 * arranger_selections_action_new_edit() for MIDI
 * functions.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_edit_midi_function (
  ArrangerSelections * sel_before,
  MidiFunctionType     midi_func_type,
  MidiFunctionOpts     opts,
  GError **            error);

/**
 * Wrapper over
 * arranger_selections_action_new_edit() for
 * automation functions.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_edit_automation_function (
  ArrangerSelections *   sel_before,
  AutomationFunctionType automation_func_type,
  GError **              error);

/**
 * Wrapper over
 * arranger_selections_action_new_edit() for
 * automation functions.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_edit_audio_function (
  ArrangerSelections * sel_before,
  AudioFunctionType    audio_func_type,
  AudioFunctionOpts    opts,
  const char *         uri,
  GError **            error);

/**
 * Creates a new action for automation autofill.
 *
 * @param region_before The region before the
 *   change.
 * @param region_after The region after the
 *   change.
 * @param already_changed Whether the change was
 *   already made.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_automation_fill (
  ZRegion * region_before,
  ZRegion * region_after,
  bool      already_changed,
  GError ** error);

/**
 * Creates a new action for splitting
 * ArrangerObject's.
 *
 * @param pos Global position to split at.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_split (
  ArrangerSelections * sel,
  const Position *     pos,
  GError **            error);

/**
 * Creates a new action for merging
 * ArrangerObject's.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_merge (ArrangerSelections * sel, GError ** error);

/**
 * Creates a new action for resizing
 * ArrangerObject's.
 *
 * @param ticks How many ticks to add to the resizing
 *   edge.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_resize (
  ArrangerSelections *               sel,
  ArrangerSelectionsActionResizeType type,
  const double                       ticks,
  const bool                         already_resized,
  GError **                          error);

/**
 * Creates a new action for quantizing
 * ArrangerObject's.
 *
 * @param opts Quantize options.
 */
WARN_UNUSED_RESULT UndoableAction *
arranger_selections_action_new_quantize (
  ArrangerSelections * sel,
  QuantizeOptions *    opts,
  GError **            error);

NONNULL ArrangerSelectionsAction *
arranger_selections_action_clone (const ArrangerSelectionsAction * src);

bool
arranger_selections_action_perform_create_or_delete (
  ArrangerSelections * sel,
  const bool           create,
  GError **            error);

#define arranger_selections_action_perform_create(sel, error) \
  arranger_selections_action_perform_create_or_delete ( \
    (ArrangerSelections *) sel, true, error)

#define arranger_selections_action_perform_delete(sel, error) \
  arranger_selections_action_perform_create_or_delete ( \
    (ArrangerSelections *) sel, false, error)

bool
arranger_selections_action_perform_record (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const bool           already_recorded,
  GError **            error);

bool
arranger_selections_action_perform_move_or_duplicate (
  ArrangerSelections *   sel,
  const bool             move,
  const double           ticks,
  const int              delta_chords,
  const int              delta_pitch,
  const int              delta_tracks,
  const int              delta_lanes,
  const double           delta_normalized_amount,
  const PortIdentifier * port_id,
  const bool             already_moved,
  GError **              error);

#define arranger_selections_action_perform_move( \
  sel, ticks, chords, pitch, tracks, lanes, norm_amt, port_id, already_moved, \
  error) \
  arranger_selections_action_perform_move_or_duplicate ( \
    (ArrangerSelections *) sel, 1, ticks, chords, pitch, tracks, lanes, \
    norm_amt, port_id, already_moved, error)
#define arranger_selections_action_perform_duplicate( \
  sel, ticks, chords, pitch, tracks, lanes, norm_amt, port_id, already_moved, \
  error) \
  arranger_selections_action_perform_move_or_duplicate ( \
    (ArrangerSelections *) sel, 0, ticks, chords, pitch, tracks, lanes, \
    norm_amt, port_id, already_moved, error)

#define arranger_selections_action_perform_move_timeline( \
  sel, ticks, delta_tracks, delta_lanes, port_id, already_moved, error) \
  arranger_selections_action_perform_move ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes, 0, port_id, already_moved, \
    error)
#define arranger_selections_action_perform_duplicate_timeline( \
  sel, ticks, delta_tracks, delta_lanes, port_id, already_moved, error) \
  arranger_selections_action_perform_duplicate ( \
    sel, ticks, 0, 0, delta_tracks, delta_lanes, 0, port_id, already_moved, \
    error)

#define arranger_selections_action_perform_move_midi( \
  sel, ticks, delta_pitch, already_moved, error) \
  arranger_selections_action_perform_move ( \
    sel, ticks, 0, delta_pitch, 0, 0, 0, NULL, already_moved, error)
#define arranger_selections_action_perform_duplicate_midi( \
  sel, ticks, delta_pitch, already_moved, error) \
  arranger_selections_action_perform_duplicate ( \
    sel, ticks, 0, delta_pitch, 0, 0, 0, NULL, already_moved, error)
#define arranger_selections_action_perform_move_chord( \
  sel, ticks, delta_chords, already_moved, error) \
  arranger_selections_action_perform_move ( \
    sel, ticks, delta_chords, 0, 0, 0, 0, NULL, already_moved, error)
#define arranger_selections_action_perform_duplicate_chord( \
  sel, ticks, delta_chords, already_moved, error) \
  arranger_selections_action_perform_duplicate ( \
    sel, ticks, delta_chords, 0, 0, 0, 0, NULL, already_moved, error)

#define arranger_selections_action_perform_move_automation( \
  sel, ticks, norm_amt, already_moved, error) \
  arranger_selections_action_perform_move ( \
    sel, ticks, 0, 0, 0, 0, norm_amt, NULL, already_moved, error)
#define arranger_selections_action_perform_duplicate_automation( \
  sel, ticks, norm_amt, already_moved, error) \
  arranger_selections_action_perform_duplicate ( \
    sel, ticks, 0, 0, 0, 0, norm_amt, NULL, already_moved, error)

bool
arranger_selections_action_perform_link (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const double         ticks,
  const int            delta_tracks,
  const int            delta_lanes,
  const bool           already_moved,
  GError **            error);

bool
arranger_selections_action_perform_edit (
  ArrangerSelections *             sel_before,
  ArrangerSelections *             sel_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error);

bool
arranger_selections_action_perform_edit_single_obj (
  const ArrangerObject *           obj_before,
  const ArrangerObject *           obj_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error);

bool
arranger_selections_action_perform_edit_midi_function (
  ArrangerSelections * sel_before,
  MidiFunctionType     midi_func_type,
  MidiFunctionOpts     opts,
  GError **            error);

bool
arranger_selections_action_perform_edit_automation_function (
  ArrangerSelections *   sel_before,
  AutomationFunctionType automation_func_type,
  GError **              error);

bool
arranger_selections_action_perform_edit_audio_function (
  ArrangerSelections * sel_before,
  AudioFunctionType    audio_func_type,
  AudioFunctionOpts    opts,
  const char *         uri,
  GError **            error);

bool
arranger_selections_action_perform_automation_fill (
  ZRegion * region_before,
  ZRegion * region_after,
  bool      already_changed,
  GError ** error);

bool
arranger_selections_action_perform_split (
  ArrangerSelections * sel,
  const Position *     pos,
  GError **            error);

bool
arranger_selections_action_perform_merge (
  ArrangerSelections * sel,
  GError **            error);

bool
arranger_selections_action_perform_resize (
  ArrangerSelections *               sel,
  ArrangerSelectionsActionResizeType type,
  const double                       ticks,
  const bool                         already_resized,
  GError **                          error);

bool
arranger_selections_action_perform_quantize (
  ArrangerSelections * sel,
  QuantizeOptions *    opts,
  GError **            error);

int
arranger_selections_action_do (ArrangerSelectionsAction * self, GError ** error);

int
arranger_selections_action_undo (
  ArrangerSelectionsAction * self,
  GError **                  error);

char *
arranger_selections_action_stringize (ArrangerSelectionsAction * self);

bool
arranger_selections_action_contains_clip (
  ArrangerSelectionsAction * self,
  AudioClip *                clip);

void
arranger_selections_action_free (ArrangerSelectionsAction * self);

/**
 * @}
 */

#endif
