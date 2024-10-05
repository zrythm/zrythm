// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Snap/grid information.
 */

#ifndef __AUDIO_SNAP_GRID_H__
#define __AUDIO_SNAP_GRID_H__

#include "common/dsp/position.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define SNAP_GRID_TIMELINE (PROJECT->snap_grid_timeline_)
#define SNAP_GRID_EDITOR (PROJECT->snap_grid_editor_)
#define SNAP_GRID_IS_EDITOR(sg) (SNAP_GRID_EDITOR == sg)
#define SNAP_GRID_IS_TIMELINE(sg) (SNAP_GRID_TIMELINE == sg)

constexpr int SNAP_GRID_DEFAULT_MAX_BAR = 10000;

enum class NoteLength
{
  NOTE_LENGTH_BAR,
  NOTE_LENGTH_BEAT,
  NOTE_LENGTH_2_1,
  NOTE_LENGTH_1_1,
  NOTE_LENGTH_1_2,
  NOTE_LENGTH_1_4,
  NOTE_LENGTH_1_8,
  NOTE_LENGTH_1_16,
  NOTE_LENGTH_1_32,
  NOTE_LENGTH_1_64,
  NOTE_LENGTH_1_128
};

const char **
note_length_get_strings (void);
const char *
note_length_to_str (NoteLength len);

enum class NoteType
{
  NOTE_TYPE_NORMAL,
  NOTE_TYPE_DOTTED, ///< 2/3 of its original size
  NOTE_TYPE_TRIPLET ///< 3/2 of its original size
};

const char **
note_type_get_strings (void);
const char *
note_type_to_str (NoteType type);

enum class NoteLengthType
{
  /** Link length with snap setting. */
  NOTE_LENGTH_LINK,

  /** Use last created object's length. */
  NOTE_LENGTH_LAST_OBJECT,

  /** Custom length. */
  NOTE_LENGTH_CUSTOM,
};

class SnapGrid final : public ISerializable<SnapGrid>
{
public:
  enum class Type
  {
    Timeline,
    Editor,
  };

public:
  // Rule of 0
  SnapGrid () = default;

  SnapGrid (Type type, NoteLength note_length, bool adaptive)
      : type_ (type), snap_adaptive_ (adaptive),
        snap_note_length_ (note_length), default_note_length_ (note_length)
  {
  }

  /**
   * @brief Returns whether any snapping is enabled.
   */
  inline bool any_snap () const { return snap_to_grid_ || snap_to_events_; }

  static int get_ticks_from_length_and_type (NoteLength length, NoteType type);

  /**
   * Gets a snap point's length in ticks.
   */
  int get_snap_ticks () const;

  /**
   * @brief Get the snap length in frames.
   */
  double get_snap_frames () const;

  /**
   * Gets a the default length in ticks.
   */
  int get_default_ticks () const;

  /**
   * Returns the grid intensity as a human-readable string.
   */
  static std::string
  stringize_length_and_type (NoteLength note_length, NoteType note_type);

  /**
   * Returns the grid intensity as a human-readable string.
   */
  std::string stringize () const;

  /**
   * Returns the next or previous SnapGrid point.
   *
   * @param[out] ret_pos Output position, if found.
   * @param pos Position to search for.
   * @param return_prev 1 to return the previous element or 0 to return the
   * next.
   *
   * @return Whether successful.
   */
  bool get_nearby_snap_point (
    Position       &ret_pos,
    const Position &pos,
    const bool      return_prev);

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  Type type_ = (Type) 0;

  /**
   * If this is on, the snap note length will be determined automatically
   * based on the current zoom level.
   *
   * The snap note type still applies.
   */
  bool snap_adaptive_ = false;

  /** Snap note length. */
  NoteLength snap_note_length_ = (NoteLength) 0;

  /** Snap note type. */
  NoteType snap_note_type_ = NoteType::NOTE_TYPE_NORMAL;

  /** Whether to snap to the grid. */
  bool snap_to_grid_ = true;

  /**
   * Whether to keep the offset when moving items.
   *
   * This requires @ref snap_to_grid to be enabled.
   */
  bool snap_to_grid_keep_offset_ = false;

  /** Whether to snap to events. */
  bool snap_to_events_ = false;

  /** Default note length. */
  NoteLength default_note_length_ = (NoteLength) 0;
  /** Default note type. */
  NoteType default_note_type_ = NoteType::NOTE_TYPE_NORMAL;

  /**
   * If this is on, the default note length will be determined automatically
   * based on the current zoom level.
   *
   * The default note type still applies.
   *
   * TODO this will be done after v1.
   */
  bool default_adaptive_ = false;

  /**
   * See NoteLengthType.
   */
  NoteLengthType length_type_ = NoteLengthType::NOTE_LENGTH_LINK;
};

/**
 * @}
 */

#endif
