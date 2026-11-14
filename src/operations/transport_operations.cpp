// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "operations/transport_operations.h"

namespace zrythm::operations
{

std::vector<Operation>
make_transport_operations ()
{
  return {
    Operation{ transport_set_play_state_operation () },
  };
}

} // namespace zrythm::operations
