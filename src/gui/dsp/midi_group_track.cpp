// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/midi_group_track.h"

MidiGroupTrack::MidiGroupTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::MidiGroup,
        PortType::Event,
        PortType::Event,
        port_registry,
        obj_registry),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity)
{
  color_ = Color (QColor ("#E66100"));
  icon_name_ = "signal-midi";
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
MidiGroupTrack::init_after_cloning (
  const MidiGroupTrack &other,
  ObjectCloneType       clone_type)
{
  FoldableTrack::copy_members_from (other, clone_type);
  ChannelTrack::copy_members_from (other, clone_type);
  ProcessableTrack::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  Track::copy_members_from (other, clone_type);
}

void
MidiGroupTrack::append_ports (std::vector<Port *> &ports, bool include_plugins)
  const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
}

bool
MidiGroupTrack::validate () const
{
  return Track::validate_base () && GroupTargetTrack::validate_base ()
         && ChannelTrack::validate_base () && AutomatableTrack::validate_base ();
}
