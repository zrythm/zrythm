// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>

#include "zrythm-config.h"

#include "dsp/midi_track.h"

MidiTrack::MidiTrack (const std::string &label, int pos)
    : Track (Track::Type::Midi, label, pos, PortType::Event, PortType::Event)
{
  color_ = Color ("#F79616");
  icon_name_ = "signal-midi";
}

bool
MidiTrack::validate () const
{
  return ChannelTrack::validate () && PianoRollTrack::validate ();
}

void
MidiTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
  RecordableTrack::init_loaded ();
  LanedTrackImpl<MidiRegion>::init_loaded ();
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

bool
MidiTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}