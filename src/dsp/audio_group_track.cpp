// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_group_track.h"

AudioGroupTrack::AudioGroupTrack (const std::string &name, int pos)
    : Track (Track::Type::AudioGroup, name, pos, PortType::Audio, PortType::Audio)
{
  /* GTK color picker color */
  color_ = Color ("#26A269");
  icon_name_ = "effect";

  generate_automation_tracks ();
}