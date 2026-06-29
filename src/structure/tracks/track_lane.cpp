// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "dsp/timebase.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/clip.h"
#include "structure/tracks/track_lane.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::tracks
{

TrackLane::TrackLane (TrackLaneDependencies dependencies, QObject * parent)
    : utils::UuidIdentifiableObject<TrackLane> (parent),
      arrangement::ArrangerObjectOwner<
        arrangement::MidiClip> (dependencies.registry_, *this),
      arrangement::ArrangerObjectOwner<
        arrangement::AudioClip> (dependencies.registry_, *this),
      soloed_lanes_exist_func_ (std::move (dependencies.soloed_lanes_exist_func_)),
      timebase_provider_ (dependencies.timebase_provider_)
{
  auto wire_model = [this] (arrangement::ArrangerObjectListModel * model) {
    QObject::connect (
      model, &arrangement::ArrangerObjectListModel::rowsInserted, this,
      [this, model] (const QModelIndex &, int first, int last) {
        for (int i = first; i <= last; ++i)
          {
            auto * clip_obj =
              qobject_cast<arrangement::Clip *> (model->object_at (i));
            if (clip_obj != nullptr)
              {
                if (auto * tp = clip_obj->timebaseProvider ())
                  tp->setSource (timebase_provider_);
              }
          }
      });
  };
  wire_model (midiClips ());
  wire_model (audioClips ());
}

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
    static_cast<const arrangement::ArrangerObjectOwner<arrangement::MidiClip> &> (
      lane));
  to_json (
    j,
    static_cast<
      const arrangement::ArrangerObjectOwner<arrangement::AudioClip> &> (lane));
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
    static_cast<arrangement::ArrangerObjectOwner<arrangement::MidiClip> &> (
      lane));
  from_json (
    j,
    static_cast<arrangement::ArrangerObjectOwner<arrangement::AudioClip> &> (
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
