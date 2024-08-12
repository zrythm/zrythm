// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/master_track.h"

MasterTrack::MasterTrack (int pos)
    : Track (Track::Type::Master, _ ("Master"), pos)
{
  /* GTK color picker color */
  color_ = Color ("#C01C28");
  icon_name_ = "effect";
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