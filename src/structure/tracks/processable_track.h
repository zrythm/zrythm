// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/automatable_track.h"
#include "structure/tracks/track_processor.h"

namespace zrythm::structure::tracks
{
class TrackProcessor;

/**
 * The ProcessableTrack class is the base class for all tracks that contain a
 * TrackProcessor.
 *
 * Tracks that want to be part of the DSP graph must at a bare minimum inherit
 * from this class.
 *
 * @see Channel and ChannelTrack for additional DSP graph functionality.
 */
class ProcessableTrack : virtual public AutomatableTrack
{
public:
  ProcessableTrack (
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    bool                             new_identity);

  ~ProcessableTrack () override = default;
  Z_DISABLE_COPY_MOVE (ProcessableTrack)

  void init_loaded (
    gui::old_dsp::plugins::PluginRegistry &plugin_registry,
    dsp::PortRegistry                     &port_registry,
    dsp::ProcessorParameterRegistry       &param_registry) override;

  /**
   * Wrapper for MIDI/instrument/chord tracks to fill in MidiEvents from the
   * timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param midi_events MidiEvents to fill.
   */
  void fill_midi_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::MidiEventVector        &midi_events);

protected:
  /**
   * Common logic for audio and MIDI/instrument tracks to fill in MidiEvents or
   * StereoPorts from the timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param stereo_ports StereoPorts to fill.
   * @param midi_events MidiEvents to fill (from Piano Roll Port for example).
   */
  void fill_events_common (
    const EngineProcessTimeInfo                                 &time_nfo,
    dsp::MidiEventVector *                                       midi_events,
    std::optional<std::pair<std::span<float>, std::span<float>>> stereo_ports)
    const;

  friend void init_from (
    ProcessableTrack       &obj,
    const ProcessableTrack &other,
    utils::ObjectCloneType  clone_type);

  void
  append_member_ports (std::vector<dsp::Port *> &ports, bool include_plugins)
    const;

private:
  static constexpr auto kProcessorKey = "processor"sv;
  friend void           to_json (nlohmann::json &j, const ProcessableTrack &p)
  {
    j[kProcessorKey] = p.processor_;
  }
  friend void from_json (const nlohmann::json &j, ProcessableTrack &p);

public:
  /**
   * The TrackProcessor, used for processing.
   *
   * This is the starting point when processing a Track.
   */
  std::unique_ptr<TrackProcessor> processor_;

protected:
  dsp::PortRegistry               &port_registry_;
  dsp::ProcessorParameterRegistry &param_registry_;
};

using ProcessableTrackVariant = std::variant<
  InstrumentTrack,
  MidiTrack,
  MasterTrack,
  MidiGroupTrack,
  AudioGroupTrack,
  MidiBusTrack,
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  ModulatorTrack>;
using ProcessableTrackPtrVariant = to_pointer_variant<ProcessableTrackVariant>;
}
