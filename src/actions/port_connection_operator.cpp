// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/port_connection_operator.h"
#include "commands/port_connection_commands.h"
#include "dsp/graph.h"
#include "dsp/port.h"
#include "structure/project/project_graph_builder.h"

namespace zrythm::actions
{

namespace
{

dsp::Port *
find_port (structure::project::Project &project, const QUuid &uuid)
{
  return qobject_cast<dsp::Port *> (
    project.get_registry ().find_by_raw_uuid (uuid));
}

}

PortConnectionOperator::PortConnectionOperator (
  undo::UndoStack             &undo_stack,
  structure::project::Project &project,
  QObject *                    parent)
    : QObject (parent), undo_stack_ (undo_stack), project_ (project)
{
}

QString
PortConnectionOperator::connectPorts (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid)
{
  const QUuid src_raw (sourcePortUuid);
  const QUuid dest_raw (destinationPortUuid);
  if (src_raw.isNull () || dest_raw.isNull ())
    return tr ("Invalid port identifier.");

  auto * src_port = find_port (project_, src_raw);
  auto * dest_port = find_port (project_, dest_raw);
  if (src_port == nullptr || dest_port == nullptr)
    return tr ("The port no longer exists.");
  if (src_port == dest_port)
    return tr ("Cannot connect a port to itself.");
  if (!src_port->is_output () || !dest_port->is_input ())
    return tr ("Connections must go from an output port to an input port.");
  if (src_port->type () != dest_port->type ())
    return tr ("Signal types must match.");

  auto               &manager = *project_.port_connections_manager_;
  const dsp::PortUuid src_id (src_raw);
  const dsp::PortUuid dest_id (dest_raw);
  if (manager.connection_exists (src_id, dest_id))
    return tr ("Those ports are already connected.");

  // Detached destination ports join the temporary validation graph only; the
  // undo command owns the final attachment state.
  const bool destination_was_detached = dest_port->detached ();
  if (destination_was_detached)
    dest_port->set_detached (false);

  QString validation_error;
  try
    {
      dsp::graph::Graph                       graph;
      structure::project::ProjectGraphBuilder builder (
        project_, project_.metronome (), project_.monitor_fader ());
      builder.build_graph (graph);

      auto * src_node = graph.get_nodes ().find_node_for_processable (*src_port);
      auto * dest_node =
        graph.get_nodes ().find_node_for_processable (*dest_port);
      if (src_node == nullptr || dest_node == nullptr)
        {
          validation_error =
            tr ("These ports are not part of the processing graph.");
        }
      else
        {
          src_node->connect_to (*dest_node);
          graph.finalize_nodes ();
          if (!graph.is_valid ())
            validation_error = tr (
              "The connection would create a cycle in the processing graph.");
        }
    }
  catch (const std::exception &error)
    {
      validation_error =
        tr ("Could not validate the connection: %1").arg (error.what ());
    }

  if (!validation_error.isEmpty ())
    {
      if (destination_was_detached)
        dest_port->set_detached (true);
      return validation_error;
    }
  if (destination_was_detached)
    dest_port->set_detached (true);

  undo_stack_.push (new commands::AddPortConnectionCommand (
    manager, src_id, dest_id, *dest_port));
  return {};
}

QString
PortConnectionOperator::disconnectPorts (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid)
{
  const QUuid src_raw (sourcePortUuid);
  const QUuid dest_raw (destinationPortUuid);
  if (src_raw.isNull () || dest_raw.isNull ())
    return tr ("Invalid port identifier.");

  auto               &manager = *project_.port_connections_manager_;
  const dsp::PortUuid src_id (src_raw);
  const dsp::PortUuid dest_id (dest_raw);
  const auto *        conn = manager.get_connection (src_id, dest_id);
  if (conn == nullptr)
    return tr ("The connection does not exist.");
  if (conn->locked_)
    return tr ("The connection is locked and cannot be removed.");
  auto * dest_port = find_port (project_, dest_raw);
  if (dest_port == nullptr)
    return tr ("The port no longer exists.");

  undo_stack_.push (new commands::RemovePortConnectionCommand (
    manager, src_id, dest_id, *dest_port));
  return {};
}

QString
PortConnectionOperator::setConnectionMultiplier (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid,
  qreal          multiplier)
{
  const QUuid src_raw (sourcePortUuid);
  const QUuid dest_raw (destinationPortUuid);
  if (src_raw.isNull () || dest_raw.isNull ())
    return tr ("Invalid port identifier.");

  auto               &manager = *project_.port_connections_manager_;
  const dsp::PortUuid src_id (src_raw);
  const dsp::PortUuid dest_id (dest_raw);
  const auto *        conn = manager.get_connection (src_id, dest_id);
  if (conn == nullptr)
    return tr ("The connection does not exist.");
  if (conn->locked_)
    return tr ("The connection is locked and cannot be edited.");

  auto * dest_port = find_port (project_, dest_raw);
  if (dest_port == nullptr)
    return tr ("The port no longer exists.");
  if (dest_port->type () == dsp::PortType::Midi)
    return tr ("MIDI connections have no adjustable strength.");

  // Audio allows double strength for gain up to +6 dB and negative gain
  // for phase inversion; CV matches the bipolar -100%…100% range shown in
  // the UI.
  const float max_abs_multiplier =
    dest_port->type () == dsp::PortType::Audio ? 2.f : 1.f;
  const float value = static_cast<float> (multiplier);
  if (value < -max_abs_multiplier || value > max_abs_multiplier)
    return tr ("The strength is out of range.");
  if (value == conn->multiplier_)
    return {};

  undo_stack_.push (new commands::UpdatePortConnectionCommand (
    manager, src_id, dest_id, value, conn->enabled_, /*merges=*/true));
  return {};
}

QString
PortConnectionOperator::setConnectionEnabled (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid,
  bool           enabled)
{
  const QUuid src_raw (sourcePortUuid);
  const QUuid dest_raw (destinationPortUuid);
  if (src_raw.isNull () || dest_raw.isNull ())
    return tr ("Invalid port identifier.");

  auto               &manager = *project_.port_connections_manager_;
  const dsp::PortUuid src_id (src_raw);
  const dsp::PortUuid dest_id (dest_raw);
  const auto *        conn = manager.get_connection (src_id, dest_id);
  if (conn == nullptr)
    return tr ("The connection does not exist.");
  if (conn->locked_)
    return tr ("The connection is locked and cannot be edited.");
  if (conn->enabled_ == enabled)
    return {};

  undo_stack_.push (new commands::UpdatePortConnectionCommand (
    manager, src_id, dest_id, conn->multiplier_, enabled, /*merges=*/false));
  return {};
}

} // namespace zrythm::actions
