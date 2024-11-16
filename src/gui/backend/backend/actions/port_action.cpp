// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/control_port.h"
#include "common/dsp/port.h"
#include "common/utils/format.h"
#include "gui/backend/backend/actions/port_action.h"

void
PortAction::init_after_cloning (const PortAction &other)
{
  UndoableAction::copy_members_from (other);
  type_ = other.type_;
  port_id_ = other.port_id_->clone_raw_ptr ();
  port_id_->setParent (this);
  val_ = other.val_;
}

PortAction::PortAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::Port)
{
}

PortAction::PortAction (
  Type                  type,
  const PortIdentifier &port_id,
  float                 val,
  bool                  is_normalized)
    : UndoableAction (UndoableAction::Type::Port), type_ (type),
      port_id_ (port_id.clone_raw_ptr ()),
      val_ (
        is_normalized
          ? Port::find_from_identifier<ControlPort> (port_id)
              ->normalized_val_to_real (val)
          : val)
{
  port_id_->setParent (this);
}

void
PortAction::do_or_undo (bool do_it)
{
  auto port = Port::find_from_identifier<ControlPort> (*port_id_);

  switch (type_)
    {
    case Type::SetControlValue:
      {
        float val_before = port->get_val ();
        port->set_control_value (val_, false, true);
        val_ = val_before;
      }
      break;
    default:
      break;
    }
}

void
PortAction::perform_impl ()
{
  do_or_undo (true);
}

void
PortAction::undo_impl ()
{
  do_or_undo (false);
}

QString
PortAction::to_string () const
{
  switch (type_)
    {
    case Type::SetControlValue:
      return format_qstr (
        QObject::tr ("Set {} to {}"), port_id_->get_label (), val_);
      break;
    }

  z_return_val_if_reached ({});
}