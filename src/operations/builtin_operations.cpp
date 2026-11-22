// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "operations/builtin_operations.h"
#include "operations/parameter_operations.h"
#include "operations/port_connection_operations.h"
#include "operations/track_operations.h"
#include "operations/transport_operations.h"

namespace zrythm::operations
{

void
register_builtin_operations (OperationRegistry &registry)
{
  registry.register_operations (make_track_operations ());
  registry.register_operations (make_transport_operations ());
  registry.register_operations (make_port_connection_operations ());
  registry.register_operations (make_parameter_operations ());
}

} // namespace zrythm::operations
