// SPDX-FileCopyrightText: © 2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_span.h"
#include "utils/format.h"

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

DEFINE_ENUM_FORMATTER (
  zrythm::structure::arrangement::AutomationFunction::Type,
  AutomationFunctionType,
  QT_TR_NOOP_UTF8 ("Flip H"),
  QT_TR_NOOP_UTF8 ("Flip V"),
  QT_TR_NOOP_UTF8 ("Flatten"));
