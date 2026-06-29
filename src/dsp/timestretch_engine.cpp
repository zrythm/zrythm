// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <stdexcept>

#include "dsp/rubberband_timestretch_engine.h"
#include "dsp/timestretch_engine.h"

namespace zrythm::dsp
{

namespace
{
// Compile-time engine registry. Display names are plain strings here; the UI
// layer is responsible for translation when presenting the dropdown.
constexpr std::array<TimeStretchEngineInfo, 1> kEngines{
  {
   { TimeStretchEngineId::RubberBand, "rubberband", "RubberBand (high quality)",
      /*supports_pitch_preserve=*/true },
   }
};
} // namespace

std::span<const TimeStretchEngineInfo>
available_timestretch_engines () noexcept
{
  return kEngines;
}

std::unique_ptr<ITimeStretchEngine>
create_timestretch_engine (
  TimeStretchEngineId  id,
  units::sample_rate_t sample_rate)
{
  switch (id)
    {
    case TimeStretchEngineId::RubberBand:
      return std::make_unique<RubberBandTimeStretchEngine> (sample_rate);
    case TimeStretchEngineId::Resampler:
      break;
    }
  throw std::invalid_argument (
    "unknown or unimplemented time-stretch engine id");
}

std::unique_ptr<ITimeStretchEngine>
create_default_timestretch_engine (
  const StretchOptions &options,
  units::sample_rate_t  sample_rate)
{
  for (const auto &info : kEngines)
    {
      if (info.id == TimeStretchEngineId::RubberBand)
        {
          auto engine =
            std::make_unique<RubberBandTimeStretchEngine> (sample_rate);
          if (engine->supports (options.algorithm))
            return engine;
        }
    }
  throw std::runtime_error (
    "no time-stretch engine supports the requested mode");
}

} // namespace zrythm::dsp
