// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/midi_bus_track.h"

namespace zrythm::structure::tracks
{
MidiBusTrack::MidiBusTrack (
  dsp::FileAudioSourceRegistry    &file_audio_source_registry,
  TrackRegistry                   &track_registry,
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  ArrangerObjectRegistry          &obj_registry,
  bool                             new_identity)
    : Track (
        Track::Type::MidiBus,
        PortType::Event,
        PortType::Event,
        plugin_registry,
        port_registry,
        param_registry,
        obj_registry),
      AutomatableTrack (
        file_audio_source_registry,
        port_registry,
        param_registry,
        new_identity),
      ProcessableTrack (port_registry, param_registry, new_identity),
      ChannelTrack (
        track_registry,
        plugin_registry,
        port_registry,
        param_registry,
        new_identity)
{
  if (new_identity)
    {
      color_ = Color (QColor ("#F5C211"));
      icon_name_ = u8"signal-midi";
      automation_tracklist_->setParent (this);
    }
}

bool
MidiBusTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
MidiBusTrack::append_ports (
  std::vector<dsp::Port *> &ports,
  bool                      include_plugins) const
{
  ProcessableTrack::append_member_ports (ports, include_plugins);
}

void
MidiBusTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry, param_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry, param_registry);
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
    static_cast<AutomatableTrack &> (obj),
    static_cast<const AutomatableTrack &> (other), clone_type);
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}

}
