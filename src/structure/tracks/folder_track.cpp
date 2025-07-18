// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "structure/tracks/folder_track.h"

namespace zrythm::structure::tracks
{
FolderTrack::FolderTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Folder,
        PortType::Unknown,
        PortType::Unknown,
        dependencies.plugin_registry_,
        dependencies.port_registry_,
        dependencies.param_registry_,
        dependencies.obj_registry_)
{
  color_ = Color (QColor ("#865E3C"));
  icon_name_ = u8"fluentui-folder-regular";
}

bool
FolderTrack::initialize ()
{
  // init_channel ();
  // generate_automation_tracks (*this);

  return true;
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
