// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/piano_roll_track.h"

namespace zrythm::structure::tracks
{
PianoRollTrackMixin::PianoRollTrackMixin (QObject * parent) : QObject (parent)
{
}

void
init_from (
  PianoRollTrackMixin       &obj,
  const PianoRollTrackMixin &other,
  utils::ObjectCloneType     clone_type)
{
  obj.drum_mode_ = other.drum_mode_;
  obj.midi_ch_ = other.midi_ch_;
  obj.passthrough_midi_input_.store (other.passthrough_midi_input_.load ());
  obj.midi_channels_ = other.midi_channels_;
}
}
