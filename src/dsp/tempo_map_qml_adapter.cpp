// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map_qml_adapter.h"

namespace zrythm::dsp
{
QString
TempoMapWrapper::getMusicalPositionString (int64_t tick) const
{
  const auto musical_pos = tempo_map_.tick_to_musical_position (tick);
  return QString::fromStdString (
    fmt::format (
      "{}.{}.{}.{:03}", musical_pos.bar, musical_pos.beat,
      musical_pos.sixteenth, musical_pos.tick));
}

TimeSignatureEventWrapper *
TempoMapWrapper::timeSignatureAtTick (int64_t tick) const
{
  const auto time_sig = tempo_map_.time_signature_at_tick (tick);
  // find corresponding time signature here
  auto it = std::ranges::find (
    timeSigEventWrappers_, time_sig.tick, &TimeSignatureEventWrapper::tick);
  if (it == timeSigEventWrappers_.end ())
    {
      if (
        !default_time_sig_
        || default_time_sig_->numerator () != time_sig.numerator
        || default_time_sig_->denominator () != time_sig.denominator)
        {
          default_time_sig_.reset (new TimeSignatureEventWrapper (time_sig));
        }
      return default_time_sig_.get ();
    }
  return *it;
}

double
TempoMapWrapper::tempoAtTick (int64_t tick) const
{
  return tempo_map_.tempo_at_tick (tick);
}

} // namespace zrythm::dsp
