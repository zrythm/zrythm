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

#include <gtk/gtk.h>
#include <jack/jack.h>

#define MAX_REGIONS 300

typedef struct AutomationTracklist AutomationTracklist;
typedef struct Region Region;
typedef struct Position Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct MidiEvents MidiEvents;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;
typedef jack_nframes_t nframes_t;

typedef enum TrackType
{
  TRACK_TYPE_INSTRUMENT,
  TRACK_TYPE_AUDIO,
  TRACK_TYPE_MASTER,
  TRACK_TYPE_CHORD,
  TRACK_TYPE_BUS
} TrackType;

/**
 * Base track struct.
 */
typedef struct Track
{
  /**
   * Useful for de/serializing.
   *
   * All IDs are stored in the tracklist.
   */
  int                 id;

  TrackType           type; ///< the type of track this is

  /**
   * Track Widget created dynamically.
   * 1 track has 1 widget.
   */
  TrackWidget *       widget;
  int                 bot_paned_visible; ///< flag to set automations visible or not
  int                 visible;
  int                 selected;
  int                 handle_pos; ///< position of multipane handle

  /**
   * Track color.
   *
   * This is used in the channels as well.
   */
  GdkRGBA              color;
} Track;

/**
 * Only to be used by implementing structs.
 *
 * Sets member variables to default values.
 */
void
track_init (Track * track);

/**
 * Wrapper.
 */
Track *
track_new (Channel * channel);

/**
 * Wrapper.
 */
void
track_add_region (Track * track,
                  Region * region);

/**
 * Wrapper to remove region, optionally freeing it.
 *
 * Free should be set to 1 when deleting and 0 when just
 * moving regions from one track to another.
 */
void
track_remove_region (Track * track,
                     Region * region,
                     int      delete);

/**
 * Returns the region at the given position, or NULL.
 */
Region *
track_get_region_at_pos (
  Track *    track,
  Position * pos);

/**
 * Wrapper.
 */
void
track_setup (Track * track);

/**
 * Returns the automation tracklist if the track type has one,
 * or NULL if it doesn't (like chord tracks).
 */
AutomationTracklist *
track_get_automation_tracklist (Track * track);

/**
 * Returns the channel of the track, if the track type has
 * a channel,
 * or NULL if it doesn't.
 */
Channel *
track_get_channel (Track * track);

/**
 * Wrapper for track types that have fader automatables.
 *
 * Otherwise returns NULL.
 */
Automatable *
track_get_fader_automatable (Track * track);

/**
 * Wrapper to get the track name.
 */
char *
track_get_name (Track * track);

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track);

#endif // __AUDIO_TRACK_H__
