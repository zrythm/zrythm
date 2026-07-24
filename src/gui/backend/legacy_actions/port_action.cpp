// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port.h"
#include "gui/backend/backend/actions/port_action.h"
#include "structure/project/project.h"
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
  obj.normalized_val_ = other.normalized_val_;
}

PortAction::PortAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::Port)
{
}

PortAction::PortAction (
  Type                          type,
  dsp::ProcessorParameter::Uuid port_id,
  float                         normalized_val)
    : UndoableAction (UndoableAction::Type::Port), type_ (type),
      port_id_ (port_id), normalized_val_ (normalized_val)
{
}

void
PortAction::do_or_undo (bool do_it)
{
// TODO
#if 0
  auto port_var = PROJECT->find_port_by_id (port_id_.value ());
  z_return_if_fail (
    port_var && std::holds_alternative<ControlPort *> (*port_var));
  auto * port = std::get<ControlPort *> (*port_var);

  switch (type_)
    {
    case Type::SetControlValue:
      {
        float val_before = port->get_val ();
        port->set_control_value (normalized_val_, false, true);
        normalized_val_ = val_before;
      }
      break;
    default:
      break;
    }
#endif
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
  return {};
// TODO
#if 0
  switch (type_)
    {
    case Type::SetControlValue:
      {
        auto port_var = PROJECT->find_port_by_id (port_id_.value ());
        z_return_val_if_fail (
          port_var && std::holds_alternative<ControlPort *> (*port_var), {});
        auto * port = std::get<ControlPort *> (*port_var);
        return format_qstr (
          QObject::tr ("Set {} to {}"), port->get_label (), normalized_val_);
      }
    }

  z_return_val_if_reached ({});
#endif
}
}
