// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_TRACK_H__
#define __AUDIO_AUDIO_TRACK_H__

#include "audio/channel_track.h"
#include "audio/track.h"

typedef struct Position        Position;
typedef struct _TrackWidget    TrackWidget;
typedef struct Channel         Channel;
typedef struct ZRegion         AudioRegion;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable     Automatable;
typedef struct StereoPorts     StereoPorts;

typedef struct Track AudioTrack;

void
audio_track_init (Track * track);

void
audio_track_setup (AudioTrack * self);

/**
 * Fills the buffers in the given StereoPorts with
 * the frames from the current clip.
 */
void
audio_track_fill_stereo_ports_from_clip (
  Track *       self,
  StereoPorts * stereo_ports,
  const long    g_start_frames,
  nframes_t     nframes);

#endif // __AUDIO_TRACK_H__
