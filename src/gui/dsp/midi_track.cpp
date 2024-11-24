// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>

#include "zrythm-config.h"

#include "gui/dsp/midi_track.h"

MidiTrack::MidiTrack (const std::string &label, int pos)
    : Track (Track::Type::Midi, label, pos, PortType::Event, PortType::Event)
{
  color_ = Color (QColor ("#F79616"));
  icon_name_ = "signal-midi";
  automation_tracklist_->setParent (this);
}

bool
MidiTrack::validate () const
{
  return Track::validate_base () && ChannelTrack::validate_base ()
         && AutomatableTrack::validate_base () && LanedTrackImpl::validate_base ();
}

void
MidiTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
  RecordableTrack::init_loaded ();
  LanedTrackImpl::init_loaded ();
  PianoRollTrack::init_loaded ();
}

void
MidiTrack::init_after_cloning (const MidiTrack &other)
{
  Track::copy_members_from (other);
  PianoRollTrack::copy_members_from (other);
  ChannelTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  RecordableTrack::copy_members_from (other);
  LanedTrackImpl::copy_members_from (other);
}

void
MidiTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
  RecordableTrack::append_member_ports (ports, include_plugins);
}

bool
MidiTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}
