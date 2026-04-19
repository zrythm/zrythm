// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

void
to_json (nlohmann::json &j, const FixedPpqTempoMap<units::PPQ>::TempoEvent &e)
{
  j = nlohmann::json{
    { "tick",  e.tick  },
    { "bpm",   e.bpm   },
    { "curve", e.curve }
  };
}

void
from_json (const nlohmann::json &j, FixedPpqTempoMap<units::PPQ>::TempoEvent &e)
{
  j.at ("tick").get_to (e.tick);
  j.at ("bpm").get_to (e.bpm);
  j.at ("curve").get_to (e.curve);
}

void
to_json (
  nlohmann::json                                         &j,
  const FixedPpqTempoMap<units::PPQ>::TimeSignatureEvent &e)
{
  j = nlohmann::json{
    { "tick",        e.tick        },
    { "numerator",   e.numerator   },
    { "denominator", e.denominator }
  };
}

void
from_json (
  const nlohmann::json                             &j,
  FixedPpqTempoMap<units::PPQ>::TimeSignatureEvent &e)
{
  j.at ("tick").get_to (e.tick);
  j.at ("numerator").get_to (e.numerator);
  j.at ("denominator").get_to (e.denominator);
}

void
to_json (nlohmann::json &j, const FixedPpqTempoMap<units::PPQ> &tempo_map)
{
  j[tempo_map.kTimeSignaturesKey] = tempo_map.time_sig_events_;
  j[tempo_map.kTempoChangesKey] = tempo_map.events_;
}

void
from_json (const nlohmann::json &j, FixedPpqTempoMap<units::PPQ> &tempo_map)
{
  j.at (tempo_map.kTimeSignaturesKey).get_to (tempo_map.time_sig_events_);
  j.at (tempo_map.kTempoChangesKey).get_to (tempo_map.events_);
  tempo_map.rebuild_cumulative_times ();
}

template class FixedPpqTempoMap<units::PPQ>;

} // namespace zrythm::dsp
