// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_bus_track.h"

MidiBusTrack::MidiBusTrack (const std::string &name, int pos)
    : Track (Track::Type::MidiBus, name, pos, PortType::Event, PortType::Event)
{
  color_ = Color ("#F5C211");
  icon_name_ = "signal-midi";
}
