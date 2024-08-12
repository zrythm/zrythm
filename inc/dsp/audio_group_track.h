// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio group track.
 */

#ifndef __AUDIO_AUDIO_GROUP_TRACK_H__
#define __AUDIO_AUDIO_GROUP_TRACK_H__

#include "dsp/automatable_track.h"
#include "dsp/foldable_track.h"
#include "dsp/group_target_track.h"

/**
 * An audio group track that can be folded and is a target for other tracks.
 * It is also an automatable track, meaning it can have automation data.
 */
class AudioGroupTrack final
    : public FoldableTrack,
      public GroupTargetTrack,
      public ICloneable<AudioGroupTrack>,
      public ISerializable<AudioGroupTrack>
{
public:
  // Rule of 0
  AudioGroupTrack () = default;
  AudioGroupTrack (const std::string &name, int pos);

  void init_after_cloning (const AudioGroupTrack &other) override
  {
    FoldableTrack::copy_members_from (other);
    ChannelTrack::copy_members_from (other);
    ProcessableTrack::copy_members_from (other);
    AutomatableTrack::copy_members_from (other);
    Track::copy_members_from (other);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();
};

#endif // __AUDIO_AUDIO_BUS_TRACK_H__
