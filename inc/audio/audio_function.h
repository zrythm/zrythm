/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
 * \file
 *
 * AUDIO functions.
 *
 * TODO move to a more appropriate directory.
 */

#ifndef __AUDIO_AUDIO_FUNCTION_H__
#define __AUDIO_AUDIO_FUNCTION_H__

#include "utils/yaml.h"

typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum AudioFunctionType
{
  AUDIO_FUNCTION_INVERT,
  AUDIO_FUNCTION_NORMALIZE,
  AUDIO_FUNCTION_REVERSE,

  /* reserved */
  AUDIO_FUNCTION_INVALID,
} AudioFunctionType;

static const cyaml_strval_t
  audio_function_type_strings[] =
{
  { __("Invert"), AUDIO_FUNCTION_INVERT },
  { __("Normalize"), AUDIO_FUNCTION_NORMALIZE },
  { __("Reverse"), AUDIO_FUNCTION_REVERSE },
  { __("Invalid"), AUDIO_FUNCTION_INVALID },
};

static inline const char *
audio_function_type_to_string (
  AudioFunctionType type)
{
  return audio_function_type_strings[type].str;
}

/**
 * Applies the given action to the given selections.
 *
 * @param sel Selections to edit.
 * @param type Function type.
 */
int
audio_function_apply (
  ArrangerSelections * sel,
  AudioFunctionType    type);

/**
 * @}
 */

#endif
