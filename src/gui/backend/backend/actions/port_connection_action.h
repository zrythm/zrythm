// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ACTION_PORT_CONNECTION_ACTION_H__
#define __ACTION_PORT_CONNECTION_ACTION_H__

#include "common/dsp/port_connection.h"
#include "common/utils/icloneable.h"
#include "gui/backend/backend/actions/undoable_action.h"

/**
 * @addtogroup actions
 *
 * @{
 */

class PortConnectionAction
    : public UndoableAction,
      public ICloneable<PortConnectionAction>,
      public ISerializable<PortConnectionAction>
{
public:
  enum class Type
  {
    Connect,
    Disconnect,
    Enable,
    Disable,
    ChangeMultiplier,
  };

public:
  PortConnectionAction ()
      : UndoableAction (UndoableAction::Type::PortConnection)
  {
  }

  PortConnectionAction (
    Type                   type,
    const PortIdentifier * src_id,
    const PortIdentifier * dest_id,
    float                  new_val);

  virtual ~PortConnectionAction () = default;

  std::string to_string () const override;

  void init_after_cloning (const PortConnectionAction &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () override { }
  void undo_impl () override;
  void perform_impl () override;

  void do_or_undo (bool do_it);

public:
  Type type_ = Type ();

  std::unique_ptr<PortConnection> connection_;

  /**
   * Value before/after the change.
   *
   * To be swapped on undo/redo.
   */
  float val_ = 0.f;
};

class PortConnectionConnectAction final : public PortConnectionAction
{
public:
  PortConnectionConnectAction (
    const PortIdentifier &src_id,
    const PortIdentifier &dest_id)
      : PortConnectionAction (Type::Connect, &src_id, &dest_id, 0.f)
  {
  }
};

class PortConnectionDisconnectAction final : public PortConnectionAction
{
public:
  PortConnectionDisconnectAction (
    const PortIdentifier &src_id,
    const PortIdentifier &dest_id)
      : PortConnectionAction (Type::Disconnect, &src_id, &dest_id, 0.f)
  {
  }
};

class PortConnectionEnableAction final : public PortConnectionAction
{
public:
  PortConnectionEnableAction (
    const PortIdentifier &src_id,
    const PortIdentifier &dest_id)
      : PortConnectionAction (Type::Enable, &src_id, &dest_id, 0.f)
  {
  }
};

class PortConnectionDisableAction final : public PortConnectionAction
{
public:
  PortConnectionDisableAction (
    const PortIdentifier &src_id,
    const PortIdentifier &dest_id)
      : PortConnectionAction (Type::Disable, &src_id, &dest_id, 0.f)
  {
  }
};

class PortConnectionChangeMultiplierAction final : public PortConnectionAction
{
public:
  PortConnectionChangeMultiplierAction (
    const PortIdentifier &src_id,
    const PortIdentifier &dest_id,
    float                 new_multiplier)
      : PortConnectionAction (Type::ChangeMultiplier, &src_id, &dest_id, new_multiplier)
  {
  }
};

/**
 * @}
 */

#endif
