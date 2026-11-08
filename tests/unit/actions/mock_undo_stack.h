// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "undo/undo_stack.h"

namespace zrythm::actions
{
static inline std::unique_ptr<undo::UndoStack>
create_mock_undo_stack ()
{
  return std::make_unique<undo::UndoStack> ([] (auto callback, bool) {
    callback ();
  });
}
}
