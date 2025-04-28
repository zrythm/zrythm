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

#include "dsp/curve.h"
#include "gui/dsp/arranger_object_span.h"
#include "utils/format.h"
#include "utils/types.h"

using namespace zrythm;

/**
 * @addtogroup dsp
 *
 * @{
 */

class MidiFunction
{
public:
  enum class Type
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

  class Options
  {
  public:
    midi_byte_t start_vel_ = 0;
    midi_byte_t end_vel_ = 0;
    midi_byte_t vel_ = 0;

    bool   ascending_ = true;
    double time_ = 0;
    double amount_ = 0;

    dsp::CurveOptions::Algorithm curve_algo_{};
    double                       curviness_{};
  };

  /**
   * Returns a string identifier for the type.
   */
  static std::string type_to_string_id (Type type);

  /**
   * Returns a string identifier for the type.
   */
  static auto string_id_to_type (const char * id) -> Type;

  /**
   * Applies the given action to the given selections.
   *
   * @param sel Selections to edit.
   * @param type Function type.
   * @throw ZrythmException on error.
   */
  static void apply (ArrangerObjectSpan sel, Type type, Options opts);
};

DEFINE_ENUM_FORMATTER (
  MidiFunction::Type,
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
