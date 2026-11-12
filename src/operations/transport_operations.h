// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "operations/operation_registry.h"

namespace zrythm::operations
{

/** The transport.* operations. */
std::vector<Operation>
make_transport_operations ();

} // namespace zrythm::operations
