/*
 * SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __MIDI_MIDI_GROUP_TRACK_H__
#define __MIDI_MIDI_GROUP_TRACK_H__

#include "dsp/channel_track.h"
#include "dsp/track.h"

typedef struct Position        Position;
typedef struct _TrackWidget    TrackWidget;
typedef struct Channel         Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable     Automatable;

void
midi_group_track_init (Track * track);

void
midi_group_track_setup (Track * self);

#endif // __MIDI_MIDI_BUS_TRACK_H__
