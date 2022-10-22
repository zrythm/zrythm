// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio group track.
 */

#ifndef __AUDIO_AUDIO_GROUP_TRACK_H__
#define __AUDIO_AUDIO_GROUP_TRACK_H__

typedef struct Track Track;

void
audio_group_track_init (Track * track);

void
audio_group_track_setup (Track * self);

#endif // __AUDIO_AUDIO_BUS_TRACK_H__
