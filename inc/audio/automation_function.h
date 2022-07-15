/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

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
 * @addtogroup audio
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
