// SPDX-FileCopyrightText: © 2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/master_track.h"

namespace zrythm::structure::tracks
{
MasterTrack::MasterTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Master,
        PortType::Audio,
        PortType::Audio,
        dependencies.to_base_dependencies ()),
      ProcessableTrack (
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      ChannelTrack (dependencies)
{
  /* GTK color picker color */
  color_ = Color (QColor ("#D90368"));
  icon_name_ = u8"jam-icons-crown";
  automationTracklist ()->setParent (this);
}

bool
MasterTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks (*this);

  return true;
}

void
MasterTrack::init_loaded (
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
  MasterTrack           &obj,
  const MasterTrack     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<ChannelTrack &> (obj),
    static_cast<const ChannelTrack &> (other), clone_type);
  init_from (
    static_cast<GroupTargetTrack &> (obj),
    static_cast<const GroupTargetTrack &> (other), clone_type);
}
}
