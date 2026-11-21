// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/port_connection_operator.h"
#include "dsp/port.h"
#include "utils/utf8_string.h"

#include "operations/port_connection_operations.h"

namespace zrythm::actions
{

namespace
{

QString
error_text (const operations::OperationReport &report)
{
  return report.applied ()
           ? QString ()
           : utils::Utf8String::from_utf8_encoded_string (report.message)
               .to_qstring ();
}

} // namespace

PortConnectionOperator::PortConnectionOperator (
  operations::OperationDispatcher &operation_dispatcher,
  QObject *                        parent)
    : QObject (parent), operation_dispatcher_ (operation_dispatcher)
{
}

std::expected<dsp::Port *, QString>
PortConnectionOperator::find_port (const QString &uuid_string) const
{
  const auto resolved = operations::resolve_registry_object<dsp::Port> (
    operation_dispatcher_.environment ().object_registry (),
    utils::Utf8String::from_qstring (uuid_string).view (), "port");
  if (!resolved.has_value ())
    {
      return std::unexpected (
        utils::Utf8String::from_utf8_encoded_string (resolved.error ().reason)
          .to_qstring ());
    }
  return resolved.value ();
}

QString
PortConnectionOperator::connectPorts (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid)
{
  const auto source = find_port (sourcePortUuid);
  if (!source.has_value ())
    return source.error ();
  const auto destination = find_port (destinationPortUuid);
  if (!destination.has_value ())
    return destination.error ();
  return error_text (operation_dispatcher_.invoke (
    operations::port_connect_operation (), source.value (),
    destination.value ()));
}

QString
PortConnectionOperator::disconnectPorts (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid)
{
  const auto source = find_port (sourcePortUuid);
  if (!source.has_value ())
    return source.error ();
  const auto destination = find_port (destinationPortUuid);
  if (!destination.has_value ())
    return destination.error ();
  return error_text (operation_dispatcher_.invoke (
    operations::port_disconnect_operation (), source.value (),
    destination.value ()));
}

QString
PortConnectionOperator::setConnectionMultiplier (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid,
  qreal          multiplier)
{
  const auto source = find_port (sourcePortUuid);
  if (!source.has_value ())
    return source.error ();
  const auto destination = find_port (destinationPortUuid);
  if (!destination.has_value ())
    return destination.error ();
  return error_text (operation_dispatcher_.invoke (
    operations::port_set_connection_multiplier_operation (), source.value (),
    destination.value (), multiplier));
}

QString
PortConnectionOperator::setConnectionEnabled (
  const QString &sourcePortUuid,
  const QString &destinationPortUuid,
  bool           enabled)
{
  const auto source = find_port (sourcePortUuid);
  if (!source.has_value ())
    return source.error ();
  const auto destination = find_port (destinationPortUuid);
  if (!destination.has_value ())
    return destination.error ();
  return error_text (operation_dispatcher_.invoke (
    operations::port_set_connection_enabled_operation (), source.value (),
    destination.value (), enabled));
}

} // namespace zrythm::actions
