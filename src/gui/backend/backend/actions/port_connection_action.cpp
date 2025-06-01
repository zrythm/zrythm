// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/project_graph_builder.h"
#include "engine/session/router.h"
#include "gui/backend/backend/actions/port_connection_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/port.h"

using namespace zrythm::gui::actions;

PortConnectionAction::PortConnectionAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::PortConnection)
{
}

void
PortConnectionAction::init_after_cloning (
  const PortConnectionAction &other,
  ObjectCloneType             clone_type)
{
  UndoableAction::copy_members_from (other, clone_type);
  type_ = other.type_;
  if (other.connection_ != nullptr)
    {
      connection_.reset (other.connection_->clone_qobject (this));
    }
  val_ = other.val_;
}

PortConnectionAction::PortConnectionAction (
  Type     type,
  PortUuid src_id,
  PortUuid dest_id,
  float    new_val)
    : type_ (type), val_ (new_val)
{
  auto * const conn = PORT_CONNECTIONS_MGR->get_connection (src_id, dest_id);
  if (conn != nullptr)
    {
      connection_.reset (conn->clone_qobject (this));
    }
  else
    connection_.reset (
      new dsp::PortConnection (src_id, dest_id, 1.f, false, true, this));
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
  auto src_var = PROJECT->find_port_by_id (connection_->src_id_);
  auto dest_var = PROJECT->find_port_by_id (connection_->dest_id_);
  z_return_if_fail (src_var && dest_var);
  std::visit (
    [&] (auto &&src, auto &&dest) {
      using SourcePortT = base_type<decltype (src)>;
      using DestPortT = base_type<decltype (dest)>;
      z_return_if_fail (src && dest);
      auto prj_connection = PORT_CONNECTIONS_MGR->get_connection (
        connection_->src_id_, connection_->dest_id_);

      switch (type_)
        {
        case Type::Connect:
        case Type::Disconnect:
          if (
            (type_ == Type::Connect && _do)
            || (type_ == Type::Disconnect && !_do))
            {
              if (!ProjectGraphBuilder::can_ports_be_connected (
                    *PROJECT, *src, *dest))
                {
                  throw ZrythmException (fmt::format (
                    "'{}' cannot be connected to '{}'", src->get_label (),
                    dest->get_label ()));
                }
              PORT_CONNECTIONS_MGR->add_connection (
                src->get_uuid (), dest->get_uuid (), 1.f, false, true);

              /* set base value if cv -> control */
              if constexpr (
                std::is_same_v<SourcePortT, CVPort>
                && std::is_same_v<DestPortT, ControlPort>)
                {
                  dest->base_value_ = dest->control_;
                }
            }
          else
            {
              PORT_CONNECTIONS_MGR->remove_connection (
                src->get_uuid (), dest->get_uuid ());
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

      /* EVENTS_PUSH (EventType::ET_PORT_CONNECTION_CHANGED, nullptr); */
    },
    *src_var, *dest_var);
}

QString
PortConnectionAction::to_string () const
{
  switch (type_)
    {
    case Type::Connect:
      return QObject::tr ("Connect ports");
    case Type::Disconnect:
      return QObject::tr ("Disconnect ports");
    case Type::Enable:
      return QObject::tr ("Enable port connection");
    case Type::Disable:
      return QObject::tr ("Disable port connection");
    case Type::ChangeMultiplier:
      return QObject::tr ("Change port connection");
    default:
      z_warn_if_reached ();
      return {};
    }
}
