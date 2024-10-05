// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_TRACK_H__
#define __AUDIO_MIDI_TRACK_H__

#include "common/dsp/automatable_track.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/piano_roll_track.h"
#include "common/utils/object_factory.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

class MidiTrack final
    : public PianoRollTrack,
      public ChannelTrack,
      public ICloneable<MidiTrack>,
      public ISerializable<MidiTrack>,
      public InitializableObjectFactory<MidiTrack>
{
  friend class InitializableObjectFactory<MidiTrack>;

public:
  void init_loaded () override;

  bool validate () const override;

  void init_after_cloning (const MidiTrack &other) override;

private:
  MidiTrack (const std::string &label = "", int pos = 0);

  bool initialize () override;

  DECLARE_DEFINE_FIELDS_METHOD ();
};

/**
 * @}
 */

#endif // __AUDIO_MIDI_TRACK_H__
