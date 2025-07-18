// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/midi_group_track.h"

namespace zrythm::structure::tracks
{
MidiGroupTrack::MidiGroupTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::MidiGroup,
        PortType::Event,
        PortType::Event,
        dependencies.plugin_registry_,
        dependencies.port_registry_,
        dependencies.param_registry_,
        dependencies.obj_registry_),
      ProcessableTrack (
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      ChannelTrack (dependencies)
{
  color_ = Color (QColor ("#E66100"));
  icon_name_ = u8"signal-midi";
  automatableTrackMixin ()->setParent (this);
}

bool
MidiGroupTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks (*this);

  return true;
}

void
MidiGroupTrack::init_loaded (
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
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
