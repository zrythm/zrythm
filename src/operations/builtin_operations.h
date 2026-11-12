// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "operations/operation_registry.h"

namespace zrythm::operations
{

/** Registers all builtin operations on the registry. */
void
register_builtin_operations (OperationRegistry &registry);

} // namespace zrythm::operations
