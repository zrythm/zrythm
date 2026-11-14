// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "operations/operation_dispatcher.h"
#include "operations/operation_environment.h"
#include "operations/operation_registry.h"

namespace zrythm::operations
{

OperationDispatcher::OperationDispatcher (
  const OperationRegistry &registry,
  OperationEnvironment    &environment,
  QObject *                parent)
    : QObject (parent), registry_ (registry), environment_ (environment)
{
}

OperationReport
OperationDispatcher::dispatch (
  std::string_view       name,
  const nlohmann::json  &args,
  const DispatchOptions &options) const
{
  return registry_.dispatch (name, args, environment_, options);
}

} // namespace zrythm::operations
