// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_lane.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::tracks
{

void
TrackLane::generate_name (size_t index)
{
  setName (format_qstr (QObject::tr (default_format_str), index + 1));
}

void
to_json (nlohmann::json &j, const TrackLane &lane)
{
  to_json (j, static_cast<const TrackLane::UuidIdentifiableObject &> (lane));
  to_json (
    j,
    static_cast<
      const arrangement::ArrangerObjectOwner<arrangement::MidiRegion> &> (lane));
  to_json (
    j,
    static_cast<const arrangement::ArrangerObjectOwner<
      arrangement::AudioRegion> &> (lane));
  j[TrackLane::kNameKey] = lane.name_;
  j[TrackLane::kHeightKey] = lane.height_;
  j[TrackLane::kMuteKey] = lane.mute_;
  j[TrackLane::kSoloKey] = lane.solo_;
  j[TrackLane::kMidiChannelKey] = lane.midi_ch_;
}
void
from_json (const nlohmann::json &j, TrackLane &lane)
{
  from_json (j, static_cast<TrackLane::UuidIdentifiableObject &> (lane));
  from_json (
    j,
    static_cast<arrangement::ArrangerObjectOwner<arrangement::MidiRegion> &> (
      lane));
  from_json (
    j,
    static_cast<arrangement::ArrangerObjectOwner<arrangement::AudioRegion> &> (
      lane));
  j.at (TrackLane::kNameKey).get_to (lane.name_);
  j.at (TrackLane::kHeightKey).get_to (lane.height_);
  j.at (TrackLane::kMuteKey).get_to (lane.mute_);
  j.at (TrackLane::kSoloKey).get_to (lane.solo_);
  j.at (TrackLane::kMidiChannelKey).get_to (lane.midi_ch_);
}

void
init_from (
  TrackLane             &obj,
  const TrackLane       &other,
  utils::ObjectCloneType clone_type)
{
  obj.name_ = other.name_;
  obj.height_ = other.height_;
  obj.mute_ = other.mute_;
  obj.solo_ = other.solo_;
  obj.midi_ch_ = other.midi_ch_;
}

TrackLane::~TrackLane () = default;
}
