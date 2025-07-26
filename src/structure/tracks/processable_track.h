// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_processor.h"

#define DEFINE_PROCESSABLE_TRACK_QML_PROPERTIES(ClassType) \
public: \
  Q_PROPERTY ( \
    zrythm::structure::tracks::AutomationTracklist * automationTracklist READ \
      automationTracklist CONSTANT)

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
  ProcessableTrack (
    const dsp::ITransport &transport,
    PortType               signal_type,
    Dependencies           dependencies);

  ~ProcessableTrack () override = default;
  Z_DISABLE_COPY_MOVE (ProcessableTrack)

  AutomationTracklist * automationTracklist () const
  {
    return automation_tracklist_.get ();
  }

  void init_loaded (
    gui::old_dsp::plugins::PluginRegistry &plugin_registry,
    dsp::PortRegistry                     &port_registry,
    dsp::ProcessorParameterRegistry       &param_registry) override;

protected:
  friend void init_from (
    ProcessableTrack       &obj,
    const ProcessableTrack &other,
    utils::ObjectCloneType  clone_type);

private:
  static constexpr auto kProcessorKey = "processor"sv;
  static constexpr auto kAutomationTracklistKey = "automationTracklist"sv;
  friend void           to_json (nlohmann::json &j, const ProcessableTrack &p)
  {
    j[kProcessorKey] = p.processor_;
    j[kAutomationTracklistKey] = p.automation_tracklist_;
  }
  friend void from_json (const nlohmann::json &j, ProcessableTrack &p);

private:
  utils::QObjectUniquePtr<AutomationTracklist> automation_tracklist_;

public:
  /**
   * The TrackProcessor, used for processing.
   *
   * This is the starting point when processing a Track.
   */
  utils::QObjectUniquePtr<TrackProcessor> processor_;
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
