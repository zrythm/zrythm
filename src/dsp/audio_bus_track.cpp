// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_bus_track.h"

AudioBusTrack::AudioBusTrack (const std::string &name, int pos)
    : Track (Track::Type::AudioBus, name, pos, PortType::Audio, PortType::Audio)
{
  /* GTK color picker color */
  color_ = Color ("#33D17A");
  icon_name_ = "effect";
}

bool
AudioBusTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
AudioBusTrack::init_after_cloning (const AudioBusTrack &other)
{
  ChannelTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  Track::copy_members_from (other);
}