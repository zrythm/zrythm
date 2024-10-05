// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATION_FUNCTION_H__
#define __AUDIO_AUTOMATION_FUNCTION_H__

#include "common/utils/format.h"
#include "common/utils/logger.h"

#include <glib/gi18n.h>

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
  N_ ("Flip H"),
  N_ ("Flip V"),
  N_ ("Flatten"));

/**
 * @}
 */

#endif
