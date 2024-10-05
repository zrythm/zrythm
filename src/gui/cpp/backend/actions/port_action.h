// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ACTION_PORT_ACTION_H__
#define __ACTION_PORT_ACTION_H__

#include "common/dsp/control_port.h"
#include "common/dsp/port_identifier.h"
#include "common/utils/icloneable.h"
#include "gui/cpp/backend/actions/undoable_action.h"

/**
 * @addtogroup actions
 *
 * @{
 */

class PortAction
    : public UndoableAction,
      public ICloneable<PortAction>,
      public ISerializable<PortAction>
{
public:
  enum class Type
  {
    /** Set control port value. */
    SetControlValue,
  };

public:
  PortAction () : UndoableAction (UndoableAction::Type::Port) { }
  virtual ~PortAction () = default;

  /**
   * @brief Construct a new action for setting a control.
   *
   * @param type
   * @param port_id
   * @param val
   * @param is_normalized
   */
  PortAction (
    Type                  type,
    const PortIdentifier &port_id,
    float                 val,
    bool                  is_normalized);

  std::string to_string () const override;

  void init_after_cloning (const PortAction &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () override { }
  void perform_impl () override;
  void undo_impl () override;
  void do_or_undo (bool do_it);

public:
  Type type_ = Type::SetControlValue;

  PortIdentifier port_id_;

  /**
   * Real (not normalized) value before/after the change.
   *
   * To be swapped on undo/redo.
   */
  float val_ = 0.0f;
};

/**
 * @brief Action for resetting a control.
 */
class PortActionResetControl : public PortAction
{
public:
  PortActionResetControl (const ControlPort &port)
      : PortAction (PortAction::Type::SetControlValue, port.id_, port.deff_, false)
  {
  }
};

/**
 * @}
 */

#endif
