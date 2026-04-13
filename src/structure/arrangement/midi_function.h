// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/curve.h"
#include "structure/arrangement/arranger_object_span.h"

namespace zrythm::structure::arrangement
{
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
  static utils::Utf8String type_to_string_id (Type type);

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
}
