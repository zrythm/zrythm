// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_bus_track.h"

AudioBusTrack::AudioBusTrack (const std::string &name, int pos)
    : Track (Track::Type::AudioBus, name, pos, PortType::Audio, PortType::Audio)
{
  /* GTK color picker color */
  color_ = Color ("#33D17A");
  icon_name_ = "effect";

  generate_automation_tracks ();
}
