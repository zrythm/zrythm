// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATION_FUNCTION_H__
#define __AUDIO_AUTOMATION_FUNCTION_H__

#include "gui/dsp/arranger_object_span.h"
#include "utils/format.h"
#include "utils/logger.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

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
  static void apply (ArrangerObjectSpanVariant sel, Type type);
};

DEFINE_ENUM_FORMATTER (
  AutomationFunction::Type,
  AutomationFunctionType,
  QT_TR_NOOP_UTF8 ("Flip H"),
  QT_TR_NOOP_UTF8 ("Flip V"),
  QT_TR_NOOP_UTF8 ("Flatten"));

/**
 * @}
 */

#endif
