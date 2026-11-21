// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <expected>

#include "dsp/port_fwd.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include "operations/operation_dispatcher.h"

namespace zrythm::actions
{

/**
 * @brief Semantic operations on user port connections.
 *
 * All mutations are dispatched through the operations layer and are
 * undoable.
 */
class PortConnectionOperator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit PortConnectionOperator (
    operations::OperationDispatcher &operation_dispatcher,
    QObject *                        parent = nullptr);

  /**
   * @brief Creates an undoable connection between two ports.
   *
   * @return Empty string on success, otherwise the refusal reason.
   */
  Q_INVOKABLE QString connectPorts (
    const QString &sourcePortUuid,
    const QString &destinationPortUuid);

  /**
   * @brief Removes an unlocked connection between two ports, undoably.
   *
   * @return Empty string on success, otherwise the refusal reason.
   */
  Q_INVOKABLE QString disconnectPorts (
    const QString &sourcePortUuid,
    const QString &destinationPortUuid);

  /**
   * @brief Changes the strength of an unlocked connection, undoably.
   *
   * The multiplier scales the source signal before it is summed into the
   * destination (or sets the gain for audio connections). MIDI connections
   * have no strength and are rejected. Consecutive changes to the same
   * connection merge into a single undo step, so drag gestures undo as
   * one action.
   *
   * @param multiplier New multiplier: -1.0…1.0 for CV connections,
   * -2.0…2.0 for audio connections. Callers must pass a finite value;
   * non-finite values are not validated.
   * @return Empty string on success, otherwise the refusal reason.
   */
  Q_INVOKABLE QString setConnectionMultiplier (
    const QString &sourcePortUuid,
    const QString &destinationPortUuid,
    qreal          multiplier);

  /**
   * @brief Enables or disables an unlocked connection, undoably.
   *
   * Disabled connections stay in the project but are skipped during
   * processing.
   *
   * @return Empty string on success, otherwise the refusal reason.
   */
  Q_INVOKABLE QString setConnectionEnabled (
    const QString &sourcePortUuid,
    const QString &destinationPortUuid,
    bool           enabled);

private:
  /**
   * @brief Resolves a port given its UUID string.
   *
   * @return The port, or the refusal text when the string is not a
   * UUID or names no registered port.
   */
  std::expected<dsp::Port *, QString>
  find_port (const QString &uuid_string) const;

  operations::OperationDispatcher &operation_dispatcher_;
};

} // namespace zrythm::actions
