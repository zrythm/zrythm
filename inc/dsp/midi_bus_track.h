// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_BUS_TRACK_H__
#define __AUDIO_MIDI_BUS_TRACK_H__

#include "dsp/channel_track.h"
#include "dsp/track.h"

typedef struct Position        Position;
typedef struct _TrackWidget    TrackWidget;
typedef struct Channel         Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable     Automatable;

typedef struct Track MidiBusTrack;

void
midi_bus_track_init (Track * track);

void
midi_bus_track_setup (Track * self);

#endif // __AUDIO_MIDI_BUS_TRACK_H__
