// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/automation_track.h"
#include "utils/uuid_identifiable_object.h"
#include "utils/variant_helpers.h"

namespace zrythm::structure::tracks
{

class Track;
class MarkerTrack;
class InstrumentTrack;
class MidiTrack;
class MasterTrack;
class MidiGroupTrack;
class AudioGroupTrack;
class FolderTrack;
class MidiBusTrack;
class AudioBusTrack;
class AudioTrack;
class ChordTrack;
class ModulatorTrack;
class LanedTrack;
class RecordableTrack;
class ProcessableTrack;
class ChannelTrack;

using TrackVariant = std::variant<
  MarkerTrack,
  InstrumentTrack,
  MidiTrack,
  MasterTrack,
  MidiGroupTrack,
  AudioGroupTrack,
  FolderTrack,
  MidiBusTrack,
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  ModulatorTrack>;
using TrackPtrVariant = to_pointer_variant<TrackVariant>;
using TrackRefVariant = to_reference_variant<TrackVariant>;
using TrackConstRefVariant = to_const_reference_variant<TrackVariant>;
using TrackUniquePtrVariant = to_unique_ptr_variant<TrackVariant>;
using OptionalTrackPtrVariant = std::optional<TrackPtrVariant>;

using TrackUuid = utils::UuidIdentifiableObject<Track>::Uuid;

using TrackResolver =
  utils::UuidIdentifiablObjectResolver<TrackPtrVariant, TrackUuid>;

template <typename TrackT>
concept AutomatableTrack = std::derived_from<TrackT, ProcessableTrack>;

template <AutomatableTrack TrackT>
inline void
generate_automation_tracks (TrackT &track)
{
  auto * mixin = track.automatableTrackMixin ();
  auto * atl = mixin->automationTracklist ();

  std::vector<utils::QObjectUniquePtr<AutomationTrack>> ats;
  if constexpr (std::derived_from<TrackT, ChannelTrack>)
    {
      auto &ch = track.channel_;
      mixin->generate_automation_tracks_for_processor (ats, *ch->fader_);
      for (auto &send : ch->sends_)
        {
          mixin->generate_automation_tracks_for_processor (ats, *send);
        }
    }

  if constexpr (std::derived_from<TrackT, ProcessableTrack>)
    {
      mixin->generate_automation_tracks_for_processor (ats, *track.processor_);
    }

  if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
    {
      const auto processors = track.get_modulator_macro_processors ();
      for (const auto &macro : processors)
        {
          mixin->generate_automation_tracks_for_processor (ats, *macro);
        }
    }

  // insert the generated automation tracks
  for (auto &at : ats)
    {
      atl->add_automation_track (std::move (at));
    }

  // mark first automation track as created & visible
  auto * ath = atl->get_first_invisible_automation_track_holder ();
  if (ath != nullptr)
    {
      ath->created_by_user_ = true;
      ath->setVisible (true);
    }

  z_debug ("generated automation tracks for '{}'", track.getName ());
}
}

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::structure::tracks::TrackUuid);
