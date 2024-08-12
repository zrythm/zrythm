// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_BUS_TRACK_H__
#define __AUDIO_MIDI_BUS_TRACK_H__

#include "dsp/channel_track.h"

/**
 * @class MidiBusTrack
 * @brief A track that processes MIDI data.
 *
 * The MidiBusTrack class is a concrete implementation of the ProcessableTrack,
 * ChannelTrack, and AutomatableTrack interfaces. It represents a track that
 * processes MIDI data.
 */
class MidiBusTrack final
    : public ChannelTrack,
      public ICloneable<MidiBusTrack>,
      public ISerializable<MidiBusTrack>
{
public:
  // Rule of 0
  MidiBusTrack () = default;
  MidiBusTrack (const std::string &name, int pos);

  void init_after_cloning (const MidiBusTrack &other) override
  {
    ChannelTrack::copy_members_from (other);
    ProcessableTrack::copy_members_from (other);
    AutomatableTrack::copy_members_from (other);
    Track::copy_members_from (other);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();
};

#endif // __AUDIO_MIDI_BUS_TRACK_H__
