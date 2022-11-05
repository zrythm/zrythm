// SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHANNEL_TRACK_H__
#define __AUDIO_CHANNEL_TRACK_H__

#include "audio/automation_tracklist.h"
#include "audio/track.h"

typedef struct Position     Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel      Channel;
typedef struct Automatable  Automatable;

/**
 * This track is for convenience. It contains common
 * variables for tracks that correspond to a channel in
 * the mixer. Should never be instantiated.
 */
typedef struct Track ChannelTrack;

void
channel_track_setup (ChannelTrack * self);

/**
 * Frees the track.
 *
 * TODO
 */
void
channel_track_free (ChannelTrack * track);

#endif // __AUDIO_CHANNEL_TRACK_H__
