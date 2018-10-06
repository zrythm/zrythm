/*
 * audio/track.h - the back end for a timeline track
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_TRACK_H__
#define __AUDIO_TRACK_H__

typedef struct Region Region;
typedef struct TrackWidget TrackWidget;
typedef struct Channel Channel;

/**
 * The track struct.
 */
typedef struct Track {
  Region          * regions[200];     ///< array of region pointers
  int             num_regions;
  TrackWidget     * widget;
  Channel         * channel;  ///< owner
} Track;

Track *
track_new (Channel * channel);

#endif // __AUDIO_TRACK_H__
