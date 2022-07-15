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
 * \file
 *
 * MIDI functions.
 *
 * TODO move to a more appropriate directory.
 */

#ifndef __AUDIO_MIDI_FUNCTION_H__
#define __AUDIO_MIDI_FUNCTION_H__

#include "utils/yaml.h"

typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum MidiFunctionType
{
  MIDI_FUNCTION_CRESCENDO,
  MIDI_FUNCTION_FLAM,
  MIDI_FUNCTION_FLIP_HORIZONTAL,
  MIDI_FUNCTION_FLIP_VERTICAL,
  MIDI_FUNCTION_LEGATO,
  MIDI_FUNCTION_PORTATO,
  MIDI_FUNCTION_STACCATO,
  MIDI_FUNCTION_STRUM,
} MidiFunctionType;

static const cyaml_strval_t midi_function_type_strings[] = {
  {__ ("Crescendo"), MIDI_FUNCTION_CRESCENDO      },
  { __ ("Flam"),     MIDI_FUNCTION_FLAM           },
  { __ ("Flip H"),   MIDI_FUNCTION_FLIP_HORIZONTAL},
  { __ ("Flip V"),   MIDI_FUNCTION_FLIP_VERTICAL  },
  { __ ("Legato"),   MIDI_FUNCTION_LEGATO         },
  { __ ("Portato"),  MIDI_FUNCTION_PORTATO        },
  { __ ("Staccato"), MIDI_FUNCTION_STACCATO       },
  { __ ("Strum"),    MIDI_FUNCTION_STRUM          },
};

static inline const char *
midi_function_type_to_string (MidiFunctionType type)
{
  return midi_function_type_strings[type].str;
}

/**
 * Applies the given action to the given selections.
 *
 * @param sel Selections to edit.
 * @param type Function type.
 */
int
midi_function_apply (
  ArrangerSelections * sel,
  MidiFunctionType     type,
  GError **            error);

/**
 * @}
 */

#endif
