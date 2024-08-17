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
MidiTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}