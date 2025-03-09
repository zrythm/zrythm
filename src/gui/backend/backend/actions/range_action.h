// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_RANGE_ACTION_H__
#define __UNDO_RANGE_ACTION_H__

#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/timeline_selections.h"
#include "gui/dsp/transport.h"

#include "dsp/position.h"

namespace zrythm::gui::actions
{

class RangeAction
    : public QObject,
      public UndoableAction,
      public ICloneable<RangeAction>,
      public zrythm::utils::serialization::ISerializable<RangeAction>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (RangeAction)

public:
  enum class Type
  {
    InsertSilence,
    Remove,
  };

  using Position = zrythm::dsp::Position;

public:
  RangeAction (QObject * parent = nullptr);
  RangeAction (
    Type      type,
    Position  start_pos,
    Position  end_pos,
    QObject * parent = nullptr);

  QString to_string () const override;

  bool can_contain_clip () const override { return true; }

  bool contains_clip (const AudioClip &clip) const override
  {
    return (sel_before_ && sel_before_->contains_clip (clip))
           || (sel_after_ && sel_after_->contains_clip (clip));
  }

  void init_after_cloning (const RangeAction &other, ObjectCloneType clone_type)
    override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () override
  {
    if (sel_before_)
      sel_before_->init_loaded (false, frames_per_tick_);
    if (sel_after_)
      sel_after_->init_loaded (false, frames_per_tick_);
    transport_->init_loaded (nullptr, nullptr);
  }

  void perform_impl () override;
  void undo_impl () override;

  void add_to_sel_after (
    std::vector<ArrangerObject *> &prj_objs,
    std::vector<ArrangerObject *> &after_objs_for_prj,
    ArrangerObject                &prj_obj,
    ArrangerObject *               after_obj);

public:
  /** Range positions. */
  Position start_pos_;
  Position end_pos_;

  /** Action type. */
  Type type_ = Type::InsertSilence;

  /** Selections before the action, starting from
   * objects intersecting with the start position and
   * ending in infinity. */
  std::unique_ptr<TimelineSelections> sel_before_;

  /** Selections after the action. */
  std::unique_ptr<TimelineSelections> sel_after_;

  /** A copy of the transport at the start of the action. */
  std::unique_ptr<Transport> transport_;

  /** Whether this is the first run. */
  bool first_run_ = false;
};

class RangeInsertSilenceAction : public RangeAction
{
public:
  RangeInsertSilenceAction (Position start_pos, Position end_pos)
      : RangeAction (Type::InsertSilence, start_pos, end_pos)
  {
  }
};

class RangeRemoveAction : public RangeAction
{
public:
  RangeRemoveAction (Position start_pos, Position end_pos)
      : RangeAction (Type::Remove, start_pos, end_pos)
  {
  }
};

}; // namespace zrythm::gui::actions

#endif
