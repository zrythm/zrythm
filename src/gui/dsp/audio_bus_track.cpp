// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/audio_bus_track.h"

AudioBusTrack::AudioBusTrack (const std::string &name, int pos)
    : Track (Track::Type::AudioBus, name, pos, PortType::Audio, PortType::Audio)
{
  /* GTK color picker color */
  color_ = Color (QColor ("#33D17A"));
  icon_name_ = "effect";
  automation_tracklist_->setParent (this);
}

bool
AudioBusTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
AudioBusTrack::append_ports (std::vector<Port *> &ports, bool include_plugins)
  const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
}

void
AudioBusTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
}

void
AudioBusTrack::init_after_cloning (const AudioBusTrack &other)
{
  ChannelTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  Track::copy_members_from (other);
}

bool
AudioBusTrack::validate () const
{
  return Track::validate_base () && ChannelTrack::validate_base ()
         && AutomatableTrack::validate_base ();
}
