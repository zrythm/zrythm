// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once
#include "structure/tracks/channel_track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief An audio bus track that can be processed and has channels.
 */
class AudioBusTrack final
    : public QObject,
      public ChannelTrack,
      public utils::InitializableObject<AudioBusTrack>
{
public:
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (AudioBusTrack)
  DEFINE_PROCESSABLE_TRACK_QML_PROPERTIES (AudioBusTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (AudioBusTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (AudioBusTrack)

public:
  friend void init_from (
    AudioBusTrack         &obj,
    const AudioBusTrack   &other,
    utils::ObjectCloneType clone_type);

  void init_loaded (
    gui::old_dsp::plugins::PluginRegistry &plugin_registry,
    dsp::PortRegistry                     &port_registry,
    dsp::ProcessorParameterRegistry       &param_registry) override;

  void temporary_virtual_method_hack () const override { }

private:
  friend void to_json (nlohmann::json &j, const AudioBusTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const ChannelTrack &> (track));
  }
  friend void from_json (const nlohmann::json &j, AudioBusTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ProcessableTrack &> (track));
    from_json (j, static_cast<ChannelTrack &> (track));
  }

  bool initialize ();
};
}
