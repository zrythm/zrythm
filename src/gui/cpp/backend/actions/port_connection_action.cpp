// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/actions/port_connection_action.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/port.h"
#include "common/dsp/router.h"

void
PortConnectionAction::init_after_cloning (const PortConnectionAction &other)
{
  UndoableAction::copy_members_from (other);
  type_ = other.type_;
  connection_ =
    other.connection_
      ? std::make_unique<PortConnection> (*other.connection_)
      : nullptr;
  val_ = other.val_;
}

PortConnectionAction::PortConnectionAction (
  Type                   type,
  const PortIdentifier * src_id,
  const PortIdentifier * dest_id,
  float                  new_val)
    : type_ (type), val_ (new_val)
{
  const auto conn = PORT_CONNECTIONS_MGR->find_connection (*src_id, *dest_id);
  if (conn)
    connection_ = std::make_unique<PortConnection> (*conn);
  else
    connection_ =
      std::make_unique<PortConnection> (*src_id, *dest_id, 1.f, false, true);
}

void
PortConnectionAction::undo_impl ()
{
  do_or_undo (false);
}

void
PortConnectionAction::perform_impl ()
{
  do_or_undo (true);
}

void
PortConnectionAction::do_or_undo (bool _do)
{
  Port * src = Port::find_from_identifier (connection_->src_id_);
  Port * dest = Port::find_from_identifier (connection_->dest_id_);
  z_return_if_fail (src && dest);
  auto prj_connection = PORT_CONNECTIONS_MGR->find_connection (
    connection_->src_id_, connection_->dest_id_);

  switch (type_)
    {
    case Type::Connect:
    case Type::Disconnect:
      if ((type_ == Type::Connect && _do) || (type_ == Type::Disconnect && !_do))
        {
          if (!src->can_be_connected_to (*dest))
            {
              throw ZrythmException (fmt::format (
                "'{}' cannot be connected to '{}'", src->get_label (),
                dest->get_label ()));
            }
          PORT_CONNECTIONS_MGR->ensure_connect (
            src->id_, dest->id_, 1.f, false, true);

          /* set base value if cv -> control */
          if (
            src->id_.type_ == PortType::CV
            && dest->id_.type_ == PortType::Control)
            {
              auto dest_control_port = dynamic_cast<ControlPort *> (dest);
              dest_control_port->base_value_ = dest_control_port->control_;
            }
        }
      else
        {
          PORT_CONNECTIONS_MGR->ensure_disconnect (src->id_, dest->id_);
        }
      ROUTER->recalc_graph (false);
      break;
    case Type::Enable:
      prj_connection->enabled_ = _do;
      break;
    case Type::Disable:
      prj_connection->enabled_ = !_do;
      break;
    case Type::ChangeMultiplier:
      {
        float val_before = prj_connection->multiplier_;
        prj_connection->multiplier_ = val_;
        val_ = val_before;
      }
      break;
    }

  EVENTS_PUSH (EventType::ET_PORT_CONNECTION_CHANGED, nullptr);
}

std::string
PortConnectionAction::to_string () const
{
  switch (type_)
    {
    case Type::Connect:
      return _ ("Connect ports");
    case Type::Disconnect:
      return _ ("Disconnect ports");
    case Type::Enable:
      return _ ("Enable port connection");
    case Type::Disable:
      return _ ("Disable port connection");
    case Type::ChangeMultiplier:
      return _ ("Change port connection");
    default:
      z_warn_if_reached ();
      return "";
    }
}