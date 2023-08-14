// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_BUS_TRACK_H__
#define __AUDIO_AUDIO_BUS_TRACK_H__

#include "dsp/channel_track.h"
#include "dsp/track.h"

typedef struct Position        Position;
typedef struct _TrackWidget    TrackWidget;
typedef struct Channel         Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable     Automatable;

typedef struct Track AudioBusTrack;

void
audio_bus_track_init (Track * track);

void
audio_bus_track_setup (AudioBusTrack * self);

#endif // __AUDIO_AUDIO_BUS_TRACK_H__
