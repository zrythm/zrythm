// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "structure/tracks/folder_track.h"

namespace zrythm::structure::tracks
{
FolderTrack::FolderTrack (
  dsp::FileAudioSourceRegistry    &file_audio_source_registry,
  TrackRegistry                   &track_registry,
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  ArrangerObjectRegistry          &obj_registry,
  bool                             new_identity)
    : Track (
        Track::Type::Folder,
        PortType::Unknown,
        PortType::Unknown,
        plugin_registry,
        port_registry,
        param_registry,
        obj_registry)
{
  if (new_identity)
    {
      color_ = Color (QColor ("#865E3C"));
      icon_name_ = u8"fluentui-folder-regular";
    }
}

bool
FolderTrack::initialize ()
{
  // init_channel ();
  // generate_automation_tracks ();

  return true;
}

void
FolderTrack::append_ports (std::vector<dsp::Port *> &ports, bool include_plugins)
  const
{
  // ProcessableTrack::append_member_ports (ports, include_plugins);
}

void
init_from (
  FolderTrack           &obj,
  const FolderTrack     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<FoldableTrack &> (obj),
    static_cast<const FoldableTrack &> (other), clone_type);
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}

void
FolderTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
}
}
