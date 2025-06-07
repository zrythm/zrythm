// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/port_action.h"
#include "gui/backend/backend/project.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/port.h"
#include "utils/format.h"

namespace zrythm::gui::actions
{

void
init_from (
  PortAction            &obj,
  const PortAction      &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<UndoableAction &> (obj),
    static_cast<const UndoableAction &> (other), clone_type);
  obj.type_ = other.type_;
  obj.port_id_ = other.port_id_;
  obj.val_ = other.val_;
}

PortAction::PortAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::Port)
{
}

PortAction::PortAction (
  Type                     type,
  PortIdentifier::PortUuid port_id,
  float                    val,
  bool                     is_normalized)
    : UndoableAction (UndoableAction::Type::Port), type_ (type), port_id_ (port_id)
{
  if (is_normalized)
    {
      auto port_var = PROJECT->find_port_by_id (port_id);
      z_return_if_fail (
        port_var.has_value ()
        && std::holds_alternative<ControlPort *> (port_var.value ()));
      val_ = std::get<ControlPort *> (*port_var)->normalized_val_to_real (val);
    }
  else
    {
      val_ = val;
    }
}

void
PortAction::do_or_undo (bool do_it)
{
  auto port_var = PROJECT->find_port_by_id (port_id_.value ());
  z_return_if_fail (
    port_var && std::holds_alternative<ControlPort *> (*port_var));
  auto * port = std::get<ControlPort *> (*port_var);

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
      {
        auto port_var = PROJECT->find_port_by_id (port_id_.value ());
        z_return_val_if_fail (
          port_var && std::holds_alternative<ControlPort *> (*port_var), {});
        auto * port = std::get<ControlPort *> (*port_var);
        return format_qstr (
          QObject::tr ("Set {} to {}"), port->get_label (), val_);
      }
    }

  z_return_val_if_reached ({});
}
}
