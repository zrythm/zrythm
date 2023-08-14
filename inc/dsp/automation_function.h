// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Automation functions.
 *
 * TODO move to a more appropriate directory.
 */

#ifndef __AUDIO_AUTOMATION_FUNCTION_H__
#define __AUDIO_AUTOMATION_FUNCTION_H__

#include "utils/yaml.h"

typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup dsp
 *
 * @{
 */

typedef enum AutomationFunctionType
{
  AUTOMATION_FUNCTION_FLIP_HORIZONTAL,
  AUTOMATION_FUNCTION_FLIP_VERTICAL,
} AutomationFunctionType;

static const cyaml_strval_t automation_function_type_strings[] = {
  {__ ("Flip H"),  AUTOMATION_FUNCTION_FLIP_HORIZONTAL},
  { __ ("Flip V"), AUTOMATION_FUNCTION_FLIP_VERTICAL  },
};

static inline const char *
automation_function_type_to_string (
  AutomationFunctionType type)
{
  return automation_function_type_strings[type].str;
}

/**
 * Applies the given action to the given selections.
 *
 * @param sel Selections to edit.
 * @param type Function type.
 */
int
automation_function_apply (
  ArrangerSelections *   sel,
  AutomationFunctionType type,
  GError **              error);

/**
 * @}
 */

#endif
