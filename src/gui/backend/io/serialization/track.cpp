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
ModulatorMacroProcessor::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("name", name_), make_field ("cvIn", cv_in_),
    make_field ("cvOut", cv_out_), make_field ("macro", macro_));
}

void
Track::define_base_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("type", type_), make_field ("name", name_),
    make_field ("iconName", icon_name_), make_field ("pos", pos_),
    make_field ("visible", visible_), make_field ("mainHeight", main_height_),
    make_field ("enabled", enabled_), make_field ("color", color_),
    make_field ("inSignalType", in_signal_type_),
    make_field ("outSignalType", out_signal_type_),
    make_field ("comment", comment_, true), make_field ("frozen", frozen_),
    make_field ("poolId", pool_id_));
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::define_base_fields (const Context &ctx)
{
  using T = ISerializable<LanedTrackImpl<TrackLaneT>>;
  T::serialize_fields (
    ctx, make_field ("lanesVisible", lanes_visible_),
    make_field ("lanesList", lanes_));
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
TrackProcessor::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("mono", mono_, true),
    make_field ("inputGain", input_gain_, true),
    make_field ("outputGain", output_gain_, true),
    make_field ("midiIn", midi_in_, true),
    make_field ("midiOut", midi_out_, true),
    make_field ("pianoRoll", piano_roll_, true),
    make_field ("monitorAudio", monitor_audio_, true),
    make_field ("stereoIn", stereo_in_, true),
    make_field ("stereoOut", stereo_out_, true),
    make_field ("midiCc", midi_cc_, true),
    make_field ("pitchBend", pitch_bend_, true),
    make_field ("polyKeyPressure", poly_key_pressure_, true),
    make_field ("channelPressure", channel_pressure_, true));
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
ProcessableTrack::define_base_fields (const Context &ctx)
{
  using T = ISerializable<ProcessableTrack>;
  T::serialize_fields (ctx, T::make_field ("processor", processor_));
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
    ctx, T::make_field ("recording", recording_),
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

  if (ctx.is_serializing ())
    {
      T::serialize_field<
        decltype (modulators_), zrythm::gui::old_dsp::plugins::PluginPtrVariant> (
        "modulators", modulators_, ctx);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);

      auto handle_plugin = [&] (yyjson_val * pl_obj, auto &plugin) {
        if (pl_obj == nullptr || yyjson_is_null (pl_obj))
          {
            plugin = nullptr;
          }
        else
          {
            auto          pl_it = yyjson_obj_iter_with (pl_obj);
            auto          setting_obj = yyjson_obj_iter_get (&pl_it, "setting");
            PluginSetting setting;
            setting.deserialize (Context (setting_obj, ctx));
            auto pl = zrythm::gui::old_dsp::plugins::Plugin::
              create_unique_from_hosting_type (setting.hosting_type_);
            std::visit (
              [&] (auto &&p) {
                using PluginT = base_type<decltype (p)>;
                p->ISerializable<PluginT>::deserialize (Context (pl_obj, ctx));
              },
              convert_to_variant<
                zrythm::gui::old_dsp::plugins::PluginPtrVariant> (pl.get ()));
            plugin = std::move (pl);
          }
      };

      auto handle_plugin_array = [&] (const auto &key, auto &plugins) {
        yyjson_val * arr = yyjson_obj_iter_get (&it, key);
        if (!arr)
          {
            throw ZrythmException ("No plugins array");
          }
        yyjson_arr_iter pl_arr_it = yyjson_arr_iter_with (arr);
        yyjson_val *    pl_obj = NULL;
        auto            size = yyjson_arr_size (arr);
        for (size_t i = 0; i < size; ++i)
          {
            pl_obj = yyjson_arr_iter_next (&pl_arr_it);
            handle_plugin (pl_obj, plugins[i]);
          }
      };

      handle_plugin_array ("modulators", modulators_);
    }

  T::serialize_fields (
    ctx, T::make_field ("modulatorMacros", modulator_macro_processors_));
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
