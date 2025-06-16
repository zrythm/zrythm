// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/uuid_identifiable_object.h"

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
}

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::structure::tracks::TrackUuid);
