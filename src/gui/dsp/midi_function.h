// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * MIDI functions.
 *
 * TODO move to a more appropriate directory.
 */

#ifndef __AUDIO_MIDI_FUNCTION_H__
#define __AUDIO_MIDI_FUNCTION_H__

#include "gui/dsp/curve.h"

#include "utils/format.h"
#include "utils/types.h"

class MidiSelections;

/**
 * @addtogroup dsp
 *
 * @{
 */

enum class MidiFunctionType
{
  Crescendo,
  Flam,
  FlipHorizontal,
  FlipVertical,
  Legato,
  Portato,
  Staccato,
  Strum,
};

class MidiFunctionOpts
{
public:
  midi_byte_t start_vel_ = 0;
  midi_byte_t end_vel_ = 0;
  midi_byte_t vel_ = 0;

  bool   ascending_ = true;
  double time_ = 0;
  double amount_ = 0;

  CurveOptions::Algorithm curve_algo_ = CurveOptions::Algorithm::Exponent;
  double                  curviness_ = 0;
};

/**
 * Returns a string identifier for the type.
 */
std::string
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
 * @throw ZrythmException on error.
 */
void
midi_function_apply (
  MidiSelections  &sel,
  MidiFunctionType type,
  MidiFunctionOpts opts);

DEFINE_ENUM_FORMATTER (
  MidiFunctionType,
  MidiFunctionType,
  QT_TR_NOOP_UTF8 ("Crescendo"),
  QT_TR_NOOP_UTF8 ("Flam"),
  QT_TR_NOOP_UTF8 ("Flip H"),
  QT_TR_NOOP_UTF8 ("Flip V"),
  QT_TR_NOOP_UTF8 ("Legato"),
  QT_TR_NOOP_UTF8 ("Portato"),
  QT_TR_NOOP_UTF8 ("Staccato"),
  QT_TR_NOOP_UTF8 ("Strum"));

/**
 * @}
 */

#endif
