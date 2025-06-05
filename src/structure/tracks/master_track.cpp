// SPDX-FileCopyrightText: © 2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/master_track.h"

namespace zrythm::structure::tracks
{
MasterTrack::MasterTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::Master,
        PortType::Audio,
        PortType::Audio,
        plugin_registry,
        port_registry,
        obj_registry),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity)
{
  if (new_identity)
    {
      /* GTK color picker color */
      color_ = Color (QColor ("#D90368"));
      icon_name_ = u8"jam-icons-crown";
    }
  automation_tracklist_->setParent (this);
}

bool
MasterTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
MasterTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
}

void
MasterTrack::init_after_cloning (
  const MasterTrack &other,
  ObjectCloneType    clone_type)
{
  Track::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  ProcessableTrack::copy_members_from (other, clone_type);
  ChannelTrack::copy_members_from (other, clone_type);
  GroupTargetTrack::copy_members_from (other, clone_type);
}

void
MasterTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
}

}
