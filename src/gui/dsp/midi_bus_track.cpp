// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/midi_bus_track.h"

MidiBusTrack::MidiBusTrack (
  TrackRegistry  &track_registry,
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry,
  bool            new_identity)
    : Track (Track::Type::MidiBus, PortType::Event, PortType::Event),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity)
{
  if (new_identity)
    {
      color_ = Color (QColor ("#F5C211"));
      icon_name_ = "signal-midi";
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
MidiBusTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
}

void
MidiBusTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
}

void
MidiBusTrack::init_after_cloning (
  const MidiBusTrack &other,
  ObjectCloneType     clone_type)
{
  ChannelTrack::copy_members_from (other, clone_type);
  ProcessableTrack::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  Track::copy_members_from (other, clone_type);
}

bool
MidiBusTrack::validate () const
{
  return Track::validate_base () && ChannelTrack::validate_base ()
         && AutomatableTrack::validate_base ();
}
