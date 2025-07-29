// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/midi_bus_track.h"

namespace zrythm::structure::tracks
{
MidiBusTrack::MidiBusTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::MidiBus,
        PortType::Midi,
        PortType::Midi,
        dependencies.to_base_dependencies ()),
      ProcessableTrack (
        dependencies.transport_,
        PortType::Midi,
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      ChannelTrack (dependencies)
{
  color_ = Color (QColor ("#F5C211"));
  icon_name_ = u8"signal-midi";
  automationTracklist ()->setParent (this);
}

bool
MidiBusTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks (*this);

  return true;
}

void
MidiBusTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry, param_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry, param_registry);
}

void
init_from (
  MidiBusTrack          &obj,
  const MidiBusTrack    &other,
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
