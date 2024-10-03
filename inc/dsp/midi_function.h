// SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * MIDI functions.
 *
 * TODO move to a more appropriate directory.
 */

#ifndef __AUDIO_MIDI_FUNCTION_H__
#define __AUDIO_MIDI_FUNCTION_H__

#include "dsp/curve.h"
#include "utils/types.h"
#include "utils/yaml.h"

#include <glib/gi18n.h>

typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup dsp
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
  { N_ ("Crescendo"), MIDI_FUNCTION_CRESCENDO       },
  { N_ ("Flam"),      MIDI_FUNCTION_FLAM            },
  { N_ ("Flip H"),    MIDI_FUNCTION_FLIP_HORIZONTAL },
  { N_ ("Flip V"),    MIDI_FUNCTION_FLIP_VERTICAL   },
  { N_ ("Legato"),    MIDI_FUNCTION_LEGATO          },
  { N_ ("Portato"),   MIDI_FUNCTION_PORTATO         },
  { N_ ("Staccato"),  MIDI_FUNCTION_STACCATO        },
  { N_ ("Strum"),     MIDI_FUNCTION_STRUM           },
};

typedef struct MidiFunctionOpts
{
  midi_byte_t start_vel;
  midi_byte_t end_vel;
  midi_byte_t vel;

  bool   ascending;
  double time;
  double amount;

  CurveAlgorithm curve_algo;
  double         curviness;

} MidiFunctionOpts;

static inline const char *
midi_function_type_to_string (MidiFunctionType type)
{
  return midi_function_type_strings[type].str;
}

/**
 * Returns a string identifier for the type.
 */
char *
midi_function_type_to_string_id (MidiFunctionType type);

/**
 * Returns a string identifier for the type.
 */
MidiFunctionType
midi_function_string_id_to_type (const char * id);

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
  MidiFunctionOpts     opts,
  GError **            error);

/**
 * @}
 */

#endif
