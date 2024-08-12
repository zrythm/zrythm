// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_group_track.h"

MidiGroupTrack::MidiGroupTrack (const std::string &name, int pos)
    : Track (Track::Type::MidiGroup, name, pos, PortType::Event, PortType::Event)
{
  color_ = Color ("#E66100");
  icon_name_ = "signal-midi";
}