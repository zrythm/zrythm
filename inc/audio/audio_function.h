/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
typedef struct Plugin             Plugin;

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum AudioFunctionType
{
  AUDIO_FUNCTION_INVERT,
  AUDIO_FUNCTION_NORMALIZE_PEAK,
  AUDIO_FUNCTION_NORMALIZE_RMS,
  AUDIO_FUNCTION_NORMALIZE_LUFS,
  AUDIO_FUNCTION_LINEAR_FADE_IN,
  AUDIO_FUNCTION_LINEAR_FADE_OUT,
  AUDIO_FUNCTION_NUDGE_LEFT,
  AUDIO_FUNCTION_NUDGE_RIGHT,
  AUDIO_FUNCTION_REVERSE,

  /** External program. */
  AUDIO_FUNCTION_EXT_PROGRAM,

  /** Guile script. */
  AUDIO_FUNCTION_GUILE_SCRIPT,

  /** Custom plugin. */
  AUDIO_FUNCTION_CUSTOM_PLUGIN,

  /* reserved */
  AUDIO_FUNCTION_INVALID,
} AudioFunctionType;

static const cyaml_strval_t audio_function_type_strings[] = {
  {__ ("Invert"),            AUDIO_FUNCTION_INVERT         },
  { __ ("Normalize peak"),   AUDIO_FUNCTION_NORMALIZE_PEAK },
  { __ ("Normalize RMS"),    AUDIO_FUNCTION_NORMALIZE_RMS  },
  { __ ("Normalize LUFS"),   AUDIO_FUNCTION_NORMALIZE_LUFS },
  { __ ("Linear fade in"),   AUDIO_FUNCTION_LINEAR_FADE_IN },
  { __ ("Linear fade out"),  AUDIO_FUNCTION_LINEAR_FADE_OUT},
  { __ ("Nudge left"),       AUDIO_FUNCTION_NUDGE_LEFT     },
  { __ ("Nudge right"),      AUDIO_FUNCTION_NUDGE_RIGHT    },
  { __ ("Reverse"),          AUDIO_FUNCTION_REVERSE        },
  { __ ("External program"), AUDIO_FUNCTION_EXT_PROGRAM    },
  { __ ("Guile script"),     AUDIO_FUNCTION_GUILE_SCRIPT   },
  { __ ("Custom plugin"),    AUDIO_FUNCTION_CUSTOM_PLUGIN  },
  { __ ("Invalid"),          AUDIO_FUNCTION_INVALID        },
};

static inline const char *
audio_function_type_to_string (AudioFunctionType type)
{
  return audio_function_type_strings[type].str;
}

char *
audio_function_get_action_target_for_type (
  AudioFunctionType type);

/**
 * Returns a detailed action name to be used for
 * actionable widgets or menus.
 *
 * @param base_action Base action to use.
 */
char *
audio_function_get_detailed_action_for_type (
  AudioFunctionType type,
  const char *      base_action);

#define audio_function_get_detailed_action_for_type_default( \
  type) \
  audio_function_get_detailed_action_for_type ( \
    type, "app.editor-function")

const char *
audio_function_get_icon_name_for_type (AudioFunctionType type);

/**
 * Returns the URI of the plugin responsible for
 * handling the type, if any.
 */
static inline const char *
audio_function_get_plugin_uri_for_type (AudioFunctionType type)
{
  switch (type)
    {
    default:
      break;
    }

  return NULL;
}

/**
 * Applies the given action to the given selections.
 *
 * This will save a file in the pool and store the
 * pool ID in the selections.
 *
 * @param sel Selections to edit.
 * @param type Function type. If invalid is passed,
 *   this will simply add the audio file in the pool
 *   for the unchanged audio material (used in
 *   audio selection actions for the selections
 *   before the change).
 *
 * @return Non-zero if error.
 */
int
audio_function_apply (
  ArrangerSelections * sel,
  AudioFunctionType    type,
  const char *         uri,
  GError **            error);

/**
 * @}
 */

#endif
