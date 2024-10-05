// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_CHORD_ACTION_H__
#define __UNDO_CHORD_ACTION_H__

#include "gui/cpp/backend/actions/undoable_action.h"
#include "gui/cpp/backend/chord_editor.h"

#include "common/dsp/chord_descriptor.h"

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Action for chord pad changes.
 */
class ChordAction final
    : public UndoableAction,
      public ICloneable<ChordAction>,
      public ISerializable<ChordAction>
{
public:
  /**
   * Type of chord action.
   */
  enum class Type
  {
    /**
     * Change single chord.
     */
    Single,

    /** Change all chords. */
    All,
  };

  ChordAction () : UndoableAction (UndoableAction::Type::Chord) { }

  /**
   * @brief Creates a new action for changing all chords.
   *
   * @param chords_before Chords before the action.
   * @param chords_after Chords after the action.
   */
  ChordAction (
    const std::vector<ChordDescriptor> &chords_before,
    const std::vector<ChordDescriptor> &chords_after);

  /**
   * @brief Creates a new action for changing a single chord.
   *
   * @param chord Chord after the change.
   * @param chord_idx Index of the chord to change.
   */
  ChordAction (const ChordDescriptor &chord, const int chord_idx);

  std::string to_string () const override;

  void init_after_cloning (const ChordAction &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () override { }
  void perform_impl () override;
  void undo_impl () override;

  void do_or_undo (bool do_it);

public:
  Type type_ = (Type) 0;

  ChordDescriptor chord_before_;
  ChordDescriptor chord_after_;
  int             chord_idx_ = 0;

  /** Chords before the change. */
  std::vector<ChordDescriptor> chords_before_;

  /** Chords after the change. */
  std::vector<ChordDescriptor> chords_after_;
};

/**
 * @}
 */

#endif
