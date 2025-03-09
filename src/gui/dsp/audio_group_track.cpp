// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/audio_group_track.h"

AudioGroupTrack::AudioGroupTrack (
  TrackRegistry  &track_registry,
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry,
  bool            new_identity)
    : Track (Track::Type::AudioGroup, PortType::Audio, PortType::Audio),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity)
{
  if (new_identity)
    {
      /* GTK color picker color */
      color_ = Color (QColor ("#26A269"));
      icon_name_ = "effect";
    }
  automation_tracklist_->setParent (this);
}

bool
AudioGroupTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
AudioGroupTrack::append_ports (std::vector<Port *> &ports, bool include_plugins)
  const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
}

bool
AudioGroupTrack::validate () const
{
  return Track::validate_base () && GroupTargetTrack::validate_base ()
         && ChannelTrack::validate_base () && AutomatableTrack::validate_base ();
}

void
AudioGroupTrack::init_loaded (
  gui::old_dsp::plugins::PluginRegistry &plugin_registry,
  PortRegistry                          &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
}

void
AudioGroupTrack::init_after_cloning (
  const AudioGroupTrack &other,
  ObjectCloneType        clone_type)
{
  FoldableTrack::copy_members_from (other, clone_type);
  ChannelTrack::copy_members_from (other, clone_type);
  ProcessableTrack::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  Track::copy_members_from (other, clone_type);
}
