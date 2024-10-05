// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_group_track.h"

AudioGroupTrack::AudioGroupTrack (const std::string &name, int pos)
    : Track (Track::Type::AudioGroup, name, pos, PortType::Audio, PortType::Audio)
{
  /* GTK color picker color */
  color_ = Color ("#26A269");
  icon_name_ = "effect";
}

bool
AudioGroupTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

bool
AudioGroupTrack::validate () const
{
  return Track::validate_base () && GroupTargetTrack::validate_base ()
         && ChannelTrack::validate_base () && AutomatableTrack::validate_base ();
}

void
AudioGroupTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
}

void
AudioGroupTrack::init_after_cloning (const AudioGroupTrack &other)
{
  FoldableTrack::copy_members_from (other);
  ChannelTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  Track::copy_members_from (other);
}