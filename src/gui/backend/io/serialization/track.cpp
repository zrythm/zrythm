// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/audio_bus_track.h"
#include "gui/dsp/audio_group_track.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/audio_track.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/folder_track.h"
#include "gui/dsp/instrument_track.h"
#include "gui/dsp/marker_track.h"
#include "gui/dsp/master_track.h"
#include "gui/dsp/midi_bus_track.h"
#include "gui/dsp/midi_group_track.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/midi_track.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/region_owner.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/track.h"
#include "gui/dsp/track_lane.h"

void
Track::define_base_fields (const Context &ctx)
{
  using T = ISerializable<Track>;
  T::call_all_base_define_fields<utils::UuidIdentifiableObject<Track>> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("name", name_),
    T::make_field ("iconName", icon_name_), T::make_field ("pos", pos_),
    T::make_field ("visible", visible_),
    T::make_field ("mainHeight", main_height_),
    T::make_field ("enabled", enabled_), T::make_field ("color", color_),
    T::make_field ("inSignalType", in_signal_type_),
    T::make_field ("outSignalType", out_signal_type_),
    T::make_field ("comment", comment_, true),
    T::make_field ("frozen", frozen_), T::make_field ("poolId", pool_id_));
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::define_base_fields (const Context &ctx)
{
  using T = ISerializable<LanedTrackImpl<TrackLaneT>>;
  T::serialize_fields (
    ctx, T::make_field ("lanesVisible", lanes_visible_),
    T::make_field ("lanesList", lanes_));
}

template void
LanedTrackImpl<MidiLane>::define_base_fields (const Context &);
template void
LanedTrackImpl<AudioLane>::define_base_fields (const Context &);

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::define_base_fields (
  const utils::serialization::ISerializableBase::Context &ctx)
{
  using T = utils::serialization::ISerializable<RegionOwnerImpl<RegionT>>;
  T::serialize_fields (ctx, T::make_field ("regionList", region_list_));
}

void
RegionList::define_fields (const ISerializableBase::Context &ctx)
{
  serialize_fields (ctx, make_field ("regions", regions_));
}

template void
RegionOwnerImpl<MidiRegion>::define_base_fields (
  const ISerializableBase::Context &);
template void
RegionOwnerImpl<AudioRegion>::define_base_fields (
  const ISerializableBase::Context &);
template void
RegionOwnerImpl<AutomationRegion>::define_base_fields (
  const ISerializableBase::Context &);
template void
RegionOwnerImpl<ChordRegion>::define_base_fields (
  const ISerializableBase::Context &);

template <typename RegionT>
void
TrackLaneImpl<RegionT>::define_base_fields (
  const utils::serialization::ISerializableBase::Context &ctx)
{
  using T = utils::serialization::ISerializable<TrackLaneImpl<RegionT>>;
  T::template call_all_base_define_fields<RegionOwnerImpl<RegionT>> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("pos", pos_), T::make_field ("name", name_),
    T::make_field ("height", height_), T::make_field ("mute", mute_),
    T::make_field ("solo", solo_), T::make_field ("midiCh", midi_ch_));
}

template void
TrackLaneImpl<MidiRegion>::define_base_fields (
  const ISerializableBase::Context &);
template void
TrackLaneImpl<AudioRegion>::define_base_fields (
  const ISerializableBase::Context &);

void
MidiLane::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiLane>;
  T::call_all_base_define_fields<TrackLaneImpl<MidiRegion>> (ctx);
}

void
AudioLane::define_fields (const Context &ctx)
{
  using T = ISerializable<AudioLane>;
  T::call_all_base_define_fields<TrackLaneImpl<AudioRegion>> (ctx);
}

void
TrackLaneList::define_fields (const Context &ctx)
{
  using T = ISerializable<TrackLaneList>;
  T::serialize_fields (ctx, T::make_field ("lanes", lanes_));
}

void
AutomationTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<AutomationTrack>;
  T::call_all_base_define_fields<RegionOwnerImplType> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("index", index_), T::make_field ("portId", port_id_),
    T::make_field ("regionList", region_list_),
    T::make_field ("created", created_),
    T::make_field ("automationMode", automation_mode_),
    T::make_field ("recordMode", record_mode_),
    T::make_field ("visible", visible_), T::make_field ("height", height_));
}

void
AutomationTracklist::define_fields (const Context &ctx)
{
  using T = ISerializable<AutomationTracklist>;
  T::serialize_fields (ctx, T::make_field ("automationTracks", ats_));
}

void
AutomatableTrack::define_base_fields (const Context &ctx)
{
  using T = ISerializable<AutomatableTrack>;
  T::serialize_fields (
    ctx, T::make_field ("automationTracklist", automation_tracklist_),
    T::make_field ("automationVisible", automation_visible_));
}

void
ChannelTrack::define_base_fields (const Context &ctx)
{
  using T = ISerializable<ChannelTrack>;
  T::serialize_fields (ctx, T::make_field ("channel", channel_));
}

void
PianoRollTrack::define_base_fields (const Context &ctx)
{
  using T = ISerializable<PianoRollTrack>;
  T::serialize_fields (
    ctx, T::make_field ("drumMode", drum_mode_),
    T::make_field ("midiCh", midi_ch_),
    T::make_field ("passthroughMidiInput", passthrough_midi_input_));
}

void
RecordableTrack::define_base_fields (const Context &ctx)
{
  using T = ISerializable<RecordableTrack>;
  T::serialize_fields (
    ctx, T::make_field ("recordingId", recording_id_),
    T::make_field ("recordSetAutomatically", record_set_automatically_));
}

void
GroupTargetTrack::define_base_fields (const Context &ctx)
{
  using T = ISerializable<GroupTargetTrack>;
  T::serialize_fields (ctx, T::make_field ("children", children_));
}

void
FoldableTrack::define_base_fields (const Context &ctx)
{
  using T = ISerializable<FoldableTrack>;
  T::serialize_fields (
    ctx, T::make_field ("size", size_), T::make_field ("folded", folded_));
}

void
MarkerTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<MarkerTrack>;
  T::call_all_base_define_fields<Track> (ctx);
  T::serialize_fields (ctx, T::make_field ("markers", markers_));
}

void
ModulatorTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<ModulatorTrack>;
  T::call_all_base_define_fields<Track, ProcessableTrack, AutomatableTrack> (
    ctx);

  T::serialize_fields (
    ctx, T::make_field ("modulators", modulators_),
    T::make_field ("modulatorMacros", modulator_macro_processors_));
}

void
MidiTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, RecordableTrack, PianoRollTrack,
    ChannelTrack, LanedTrackImpl<MidiLane>> (ctx);
}

void
InstrumentTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<InstrumentTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, RecordableTrack, PianoRollTrack,
    ChannelTrack, LanedTrackImpl<MidiLane>, GroupTargetTrack> (ctx);
}

void
TempoTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<TempoTrack>;
  T::call_all_base_define_fields<Track, AutomatableTrack> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("bpmPort", bpm_port_),
    T::make_field ("beatsPerBarPort", beats_per_bar_port_),
    T::make_field ("beatUnitPort", beat_unit_port_));
}

void
AudioTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<AudioTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, RecordableTrack, ChannelTrack,
    LanedTrackImpl<AudioLane>> (ctx);
}

void
AudioBusTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<AudioBusTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, ChannelTrack> (ctx);
}

void
MidiBusTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiBusTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, ChannelTrack> (ctx);
}

void
AudioGroupTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<AudioGroupTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, ChannelTrack, GroupTargetTrack,
    FoldableTrack> (ctx);
}

void
MidiGroupTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiGroupTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, ChannelTrack, GroupTargetTrack,
    FoldableTrack> (ctx);
}

void
MasterTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<MasterTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, ChannelTrack, GroupTargetTrack> (
    ctx);
}

void
ChordTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<ChordTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, ChannelTrack, RecordableTrack,
    RegionOwnerImpl<ChordRegion>> (ctx);
  T::serialize_fields (ctx, T::make_field ("scaleObjects", scales_));
}

void
FolderTrack::define_fields (const Context &ctx)
{
  using T = ISerializable<FolderTrack>;
  T::call_all_base_define_fields<
    Track, ProcessableTrack, AutomatableTrack, ChannelTrack, FoldableTrack> (
    ctx);
}
