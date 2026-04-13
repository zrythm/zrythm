// SPDX-FileCopyrightText: © 2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_span.h"

namespace zrythm::structure::arrangement
{

class AutomationFunction
{
public:
  enum class Type
  {
    FlipHorizontal,
    FlipVertical,
    Flatten,
  };

  /**
   * Applies the given action to the given selections.
   *
   * @param sel Selections to edit.
   * @param type Function type.
   * @throw ZrythmException on error.
   */
  static void apply (ArrangerObjectSpan sel, Type type);
};

}
