// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "operations/track_operations.h"

namespace zrythm::operations
{

std::vector<Operation>
make_track_operations ()
{
  return {
    Operation{ track_set_name_operation () },
    Operation{ track_set_color_operation () },
    Operation{ track_add_operation () },
    Operation{ track_list_operation () },
  };
}

} // namespace zrythm::operations
