// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __MIDI_MIDI_GROUP_TRACK_H__
#define __MIDI_MIDI_GROUP_TRACK_H__

#include "dsp/automatable_track.h"
#include "dsp/foldable_track.h"
#include "dsp/group_target_track.h"

class MidiGroupTrack final
    : public FoldableTrack,
      public GroupTargetTrack,
      public ICloneable<MidiGroupTrack>,
      public ISerializable<MidiGroupTrack>
{
public:
  // Rule of 0
  MidiGroupTrack () = default;
  MidiGroupTrack (const std::string &name, int pos);

  void init_after_cloning (const MidiGroupTrack &other) override
  {
    FoldableTrack::copy_members_from (other);
    ChannelTrack::copy_members_from (other);
    ProcessableTrack::copy_members_from (other);
    AutomatableTrack::copy_members_from (other);
    Track::copy_members_from (other);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();
};

#endif // __MIDI_MIDI_BUS_TRACK_H__
