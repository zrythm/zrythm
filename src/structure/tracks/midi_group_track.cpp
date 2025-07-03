// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/midi_group_track.h"

namespace zrythm::structure::tracks
{
MidiGroupTrack::MidiGroupTrack (
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  TrackRegistry                &track_registry,
  PluginRegistry               &plugin_registry,
  PortRegistry                 &port_registry,
  ArrangerObjectRegistry       &obj_registry,
  bool                          new_identity)
    : Track (
        Track::Type::MidiGroup,
        PortType::Event,
        PortType::Event,
        plugin_registry,
        port_registry,
        obj_registry),
      AutomatableTrack (file_audio_source_registry, port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity)
{
  color_ = Color (QColor ("#E66100"));
  icon_name_ = u8"signal-midi";
  automation_tracklist_->setParent (this);
}

bool
MidiGroupTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
MidiGroupTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
}

void
init_from (
  MidiGroupTrack        &obj,
  const MidiGroupTrack  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<FoldableTrack &> (obj),
    static_cast<const FoldableTrack &> (other), clone_type);
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

void
MidiGroupTrack::append_ports (std::vector<Port *> &ports, bool include_plugins)
  const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
}
}
