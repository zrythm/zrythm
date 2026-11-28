// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "operations/operation.h"
#include "operations/operation_registry.h"
#include "operations/typed_operation.h"
#include <nlohmann/json.hpp>

namespace zrythm::operations
{

/**
 * Wraps an operation registry and environment in a QObject.
 *
 * Both dispatch by id and typed invocation run against the wrapped
 * environment.
 */
class OperationDispatcher : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  OperationDispatcher (
    const OperationRegistry &registry,
    OperationEnvironment    &environment,
    QObject *                parent = nullptr);

  /**
   * Looks up the operation by id and runs it against the environment.
   *
   * An unknown id is an Error report; anything the operation rejects
   * is a Refused report. An Edit without an expected revision is refused
   * with MissingRevision.
   */
  [[nodiscard]] OperationReport dispatch (
    OperationId            id,
    const nlohmann::json  &args = nlohmann::json::object (),
    const DispatchOptions &options = {}) const;

  /**
   * The session environment operations run against.
   */
  OperationEnvironment &environment () const { return environment_; }

  /**
   * Runs the given typed operation against the environment.
   *
   * The values are checked against the operation's handler signature at
   * compile time; renaming the operation or changing its parameters
   * breaks callers at compile time.
   */
  template <typename Handler, std::size_t NumParams, typename... Values>
  [[nodiscard]] OperationReport invoke (
    const TypedOperation<Handler, NumParams> &operation,
    Values &&... values) const
  {
    return operation.invoke_typed (
      environment_, std::forward<Values> (values)...);
  }

private:
  const OperationRegistry &registry_;
  OperationEnvironment    &environment_;
};

} // namespace zrythm::operations
