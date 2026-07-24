// SPDX-FileCopyrightText: © 2020-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/transport.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "structure/arrangement/arranger_object_span.h"

namespace zrythm::gui::actions
{

class RangeAction : public QObject, public UndoableAction
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (RangeAction)

  using ArrangerObjectSpan = structure::arrangement::ArrangerObjectSpan;
  using ArrangerObject = structure::arrangement::ArrangerObject;
  using Transport = dsp::Transport;

public:
  enum class Type
  {
    InsertSilence,
    Remove,
  };

public:
  RangeAction (QObject * parent = nullptr);
  RangeAction (
    Type           type,
    signed_frame_t start_pos,
    signed_frame_t end_pos,
    QObject *      parent = nullptr);

  QString to_string () const override;

  double get_range_size_in_ticks () const
  {
    throw std::runtime_error ("Not implemented");
    // return end_pos_.ticks_ - start_pos_.ticks_;
  }

  friend void init_from (
    RangeAction           &obj,
    const RangeAction     &other,
    utils::ObjectCloneType clone_type);

private:
  void init_loaded_impl () override;

  void perform_impl () override;
  void undo_impl () override;

  ArrangerObjectSpan get_before_objects () const;

public:
  /** Range positions in frames. */
  signed_frame_t start_pos_{};
  signed_frame_t end_pos_{};

  /** Action type. */
  Type type_ = Type::InsertSilence;

  /** Selections before the action, starting from objects intersecting with the
   * start position and ending in infinity. */
  std::vector<ArrangerObject::Uuid> affected_objects_before_;

  /**
   * @brief Objects removed from the project while performing the action.
   *
   * This is a subset of affected_objects_before_.
   *
   * These objects will be added back to the project on undo.
   */
  std::vector<ArrangerObject::Uuid> objects_removed_;

  /**
   * @brief Objects added to the project while performing the action.
   *
   * These objects will be removed on undo.
   */
  std::vector<ArrangerObject::Uuid> objects_added_;

  /**
   * @brief Objects moved (not added/removed) during the action.
   *
   * This is a subset of affected_objects_before_.
   *
   * These objects will be moved back to their original positions on undo.
   */
  std::vector<ArrangerObject::Uuid> objects_moved_;

  /** Selections after the action. */
  // std::vector<ArrangerObject::Uuid> sel_after_;

  /** A copy of the transport at the start of the action. */
  std::unique_ptr<Transport> transport_;

  /** Whether this is the first run. */
  bool first_run_ = false;
};

class RangeInsertSilenceAction : public RangeAction
{
public:
  RangeInsertSilenceAction (signed_frame_t start_pos, signed_frame_t end_pos)
      : RangeAction (Type::InsertSilence, start_pos, end_pos)
  {
  }
};

class RangeRemoveAction : public RangeAction
{
public:
  RangeRemoveAction (signed_frame_t start_pos, signed_frame_t end_pos)
      : RangeAction (Type::Remove, start_pos, end_pos)
  {
  }
};

}; // namespace zrythm::gui::actions
