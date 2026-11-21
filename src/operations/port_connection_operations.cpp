// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "operations/port_connection_operations.h"

namespace zrythm::operations
{

std::vector<Operation>
make_port_connection_operations ()
{
  return {
    Operation{ port_connect_operation () },
    Operation{ port_disconnect_operation () },
    Operation{ port_set_connection_multiplier_operation () },
    Operation{ port_set_connection_enabled_operation () },
  };
}

} // namespace zrythm::operations
