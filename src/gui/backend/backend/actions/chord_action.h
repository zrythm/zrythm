// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_CHORD_ACTION_H__
#define __UNDO_CHORD_ACTION_H__

#include "dsp/chord_descriptor.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/chord_editor.h"

namespace zrythm::gui::actions
{

/**
 * Action for chord pad changes.
 */
class ChordAction final
    : public QObject,
      public UndoableAction,
      public ICloneable<ChordAction>,
      public utils::serialization::ISerializable<ChordAction>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (ChordAction)

public:
  using ChordDescriptor = dsp::ChordDescriptor;

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

  ChordAction (QObject * parent = nullptr);

  /**
   * @brief Creates a new action for changing all chords.
   *
   * @param chords_before Chords before the action.
   * @param chords_after Chords after the action.
   */
  ChordAction (
    const std::vector<ChordDescriptor> &chords_before,
    const std::vector<ChordDescriptor> &chords_after,
    QObject *                           parent = nullptr);

  /**
   * @brief Creates a new action for changing a single chord.
   *
   * @param chord Chord after the change.
   * @param chord_idx Index of the chord to change.
   */
  ChordAction (
    const ChordDescriptor &chord,
    int                    chord_idx,
    QObject *              parent = nullptr);

  QString to_string () const override;

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

}; // namespace zrythm::gui::actions

#endif
