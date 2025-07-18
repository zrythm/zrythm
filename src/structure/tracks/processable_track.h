// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/automatable_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_processor.h"

#define DEFINE_PROCESSABLE_TRACK_QML_PROPERTIES(ClassType) \
public: \
  Q_PROPERTY ( \
    structure::tracks::AutomatableTrackMixin * automatableTrackMixin READ \
      automatableTrackMixin CONSTANT)

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
class ProcessableTrack : virtual public Track
{
public:
  using Dependencies = AutomationTrackHolder::Dependencies;
  ProcessableTrack (Dependencies dependencies);

  ~ProcessableTrack () override = default;
  Z_DISABLE_COPY_MOVE (ProcessableTrack)

  AutomatableTrackMixin * automatableTrackMixin () const
  {
    return automatable_track_mixin_.get ();
  }

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
    dsp::MidiEventVector        &midi_events)
  {
    fill_events_common (time_nfo, &midi_events, std::nullopt);
  }

  /**
   * Wrapper for audio tracks to fill in StereoPorts from the timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param stereo_ports StereoPorts to fill.
   */
  void fill_audio_events (
    const EngineProcessTimeInfo                  &time_nfo,
    std::pair<std::span<float>, std::span<float>> stereo_ports)
  {
    fill_events_common (time_nfo, nullptr, stereo_ports);
  }

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

private:
  static constexpr auto kProcessorKey = "processor"sv;
  static constexpr auto kAutomatableTrackMixinKey = "automatableTrackMixin"sv;
  friend void           to_json (nlohmann::json &j, const ProcessableTrack &p)
  {
    j[kProcessorKey] = p.processor_;
    j[kAutomatableTrackMixinKey] = p.automatable_track_mixin_;
  }
  friend void from_json (const nlohmann::json &j, ProcessableTrack &p);

private:
  utils::QObjectUniquePtr<AutomatableTrackMixin> automatable_track_mixin_;

public:
  /**
   * The TrackProcessor, used for processing.
   *
   * This is the starting point when processing a Track.
   */
  utils::QObjectUniquePtr<TrackProcessor> processor_;

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
