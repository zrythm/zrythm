// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/master_track.h"

MasterTrack::MasterTrack (int pos)
    : Track (Track::Type::Master, _ ("Master"), pos, PortType::Audio, PortType::Audio)
{
  /* GTK color picker color */
  color_ = Color (QColor ("#D90368"));
  icon_name_ = "jam-icons-crown";
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
MasterTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
}

void
MasterTrack::init_after_cloning (const MasterTrack &other)
{
  Track::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  ChannelTrack::copy_members_from (other);
  GroupTargetTrack::copy_members_from (other);
}

void
MasterTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
}

bool
MasterTrack::validate () const
{
  return Track::validate_base () && GroupTargetTrack::validate_base ()
         && ChannelTrack::validate_base () && AutomatableTrack::validate_base ();
}