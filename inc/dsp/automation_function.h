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

#include <glib/gi18n.h>

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
  AUTOMATION_FUNCTION_FLATTEN,
} AutomationFunctionType;

static const char * automation_function_type_strings[] = {
  N_ ("Flip H"),
  N_ ("Flip V"),
  N_ ("Flatten"),
};

static inline const char *
automation_function_type_to_string (AutomationFunctionType type)
{
  return automation_function_type_strings[type];
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
