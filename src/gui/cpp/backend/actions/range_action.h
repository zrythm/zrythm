// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_RANGE_ACTION_H__
#define __UNDO_RANGE_ACTION_H__

#include "gui/cpp/backend/actions/undoable_action.h"
#include "gui/cpp/backend/timeline_selections.h"

#include "common/dsp/position.h"
#include "common/dsp/transport.h"

/**
 * @addtogroup actions
 *
 * @{
 */

class RangeAction
    : public UndoableAction,
      public ICloneable<RangeAction>,
      public ISerializable<RangeAction>
{
public:
  enum class Type
  {
    InsertSilence,
    Remove,
  };

public:
  RangeAction ();
  RangeAction (Type type, Position start_pos, Position end_pos);

  std::string to_string () const override;

  bool can_contain_clip () const override { return true; }

  bool contains_clip (const AudioClip &clip) const override
  {
    return (sel_before_ && sel_before_->contains_clip (clip))
           || (sel_after_ && sel_after_->contains_clip (clip));
  }

  void init_after_cloning (const RangeAction &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () override
  {
    if (sel_before_)
      sel_before_->init_loaded (false, this);
    if (sel_after_)
      sel_after_->init_loaded (false, this);
    transport_->init_loaded (nullptr, nullptr);
  }

  void perform_impl () override;
  void undo_impl () override;

  void add_to_sel_after (
    std::vector<ArrangerObject *>                &prj_objs,
    std::vector<std::shared_ptr<ArrangerObject>> &after_objs_for_prj,
    ArrangerObject                               &prj_obj,
    std::shared_ptr<ArrangerObject>             &&after_obj);

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

/**
 * @}
 */

#endif
