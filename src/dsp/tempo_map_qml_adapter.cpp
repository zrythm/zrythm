// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map_qml_adapter.h"

namespace zrythm::dsp
{
QString
TempoMapWrapper::getMusicalPositionString (int64_t tick) const
{
  const auto musical_pos =
    tempo_map_.tick_to_musical_position (units::ticks (tick));
  return QString::fromStdString (
    fmt::format (
      "{}.{}.{}.{:03}", musical_pos.bar, musical_pos.beat,
      musical_pos.sixteenth, musical_pos.tick));
}

int
TempoMapWrapper::timeSignatureNumeratorAtTick (int64_t tick) const
{
  const auto time_sig = tempo_map_.time_signature_at_tick (units::ticks (tick));
  return time_sig.numerator;
}

int
TempoMapWrapper::timeSignatureDenominatorAtTick (int64_t tick) const
{
  const auto time_sig = tempo_map_.time_signature_at_tick (units::ticks (tick));
  return time_sig.denominator;
}

double
TempoMapWrapper::tempoAtTick (int64_t tick) const
{
  return tempo_map_.tempo_at_tick (units::ticks (tick));
}

} // namespace zrythm::dsp
