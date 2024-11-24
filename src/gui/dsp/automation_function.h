// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATION_FUNCTION_H__
#define __AUDIO_AUTOMATION_FUNCTION_H__

#include "utils/format.h"
#include "utils/logger.h"

class AutomationSelections;

/**
 * @addtogroup dsp
 *
 * @{
 */

enum class AutomationFunctionType
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
void
automation_function_apply (
  AutomationSelections  &sel,
  AutomationFunctionType type);

DEFINE_ENUM_FORMATTER (
  AutomationFunctionType,
  AutomationFunctionType,
  QT_TR_NOOP_UTF8 ("Flip H"),
  QT_TR_NOOP_UTF8 ("Flip V"),
  QT_TR_NOOP_UTF8 ("Flatten"));

/**
 * @}
 */

#endif
