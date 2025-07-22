// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/audio_bus_track.h"

namespace zrythm::structure::tracks
{
AudioBusTrack::AudioBusTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::AudioBus,
        PortType::Audio,
        PortType::Audio,
        dependencies.to_base_dependencies ()),
      ProcessableTrack (
        dependencies.transport_,
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      ChannelTrack (dependencies)
{
  /* GTK color picker color */
  color_ = Color (QColor ("#33D17A"));
  icon_name_ = u8"effect";
  automationTracklist ()->setParent (this);
}

bool
AudioBusTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks (*this);

  return true;
}

void
AudioBusTrack::init_loaded (
  gui::old_dsp::plugins::PluginRegistry &plugin_registry,
  dsp::PortRegistry                     &port_registry,
  dsp::ProcessorParameterRegistry       &param_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry, param_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry, param_registry);
}

void
init_from (
  AudioBusTrack         &obj,
  const AudioBusTrack   &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ChannelTrack &> (obj),
    static_cast<const ChannelTrack &> (other), clone_type);
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
