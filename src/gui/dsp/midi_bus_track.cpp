// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/midi_bus_track.h"

MidiBusTrack::MidiBusTrack (const std::string &name, int pos)
    : Track (Track::Type::MidiBus, name, pos, PortType::Event, PortType::Event)
{
  color_ = Color (QColor ("#F5C211"));
  icon_name_ = "signal-midi";
  automation_tracklist_->setParent (this);
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
MidiBusTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
}

void
MidiBusTrack::init_after_cloning (const MidiBusTrack &other)
{
  ChannelTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  Track::copy_members_from (other);
}

bool
MidiBusTrack::validate () const
{
  return Track::validate_base () && ChannelTrack::validate_base ()
         && AutomatableTrack::validate_base ();
}
