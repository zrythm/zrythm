// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "operations/parameter_operations.h"

namespace zrythm::operations
{

std::vector<Operation>
make_parameter_operations ()
{
  return {
    Operation{ parameter_set_value_operation () },
  };
}

} // namespace zrythm::operations
