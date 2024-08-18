// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * The master track.
 */

#ifndef __AUDIO_MASTER_TRACK_H__
#define __AUDIO_MASTER_TRACK_H__

#include "dsp/group_target_track.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define P_MASTER_TRACK (TRACKLIST->master_track_)

class MasterTrack final
    : public GroupTargetTrack,
      public ICloneable<MasterTrack>,
      public ISerializable<MasterTrack>,
      public InitializableObjectFactory<MasterTrack>
{
public:
  friend class InitializableObjectFactory<MasterTrack>;

  void init_after_cloning (const MasterTrack &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  MasterTrack (int pos = 0);

  bool initialize () override;
};

/**
 * @}
 */

#endif // __AUDIO_MASTER_TRACK_H__
