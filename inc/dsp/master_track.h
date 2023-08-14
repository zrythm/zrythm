// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * The master track.
 */

#ifndef __AUDIO_MASTER_TRACK_H__
#define __AUDIO_MASTER_TRACK_H__

#include "dsp/audio_bus_track.h"
#include "dsp/track.h"

typedef struct Position        Position;
typedef struct _TrackWidget    TrackWidget;
typedef struct Channel         Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable     Automatable;
typedef struct Track           MasterTrack;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define P_MASTER_TRACK (TRACKLIST->master_track)

void
master_track_init (Track * track);

void
master_track_setup (Track * self);

/**
 * @}
 */

#endif // __AUDIO_MASTER_TRACK_H__
