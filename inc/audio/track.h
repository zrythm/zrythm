/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * The backend for a timeline track.
 */

#ifndef __AUDIO_TRACK_H__
#define __AUDIO_TRACK_H__

#include "audio/automation_tracklist.h"
#include "audio/scale.h"
#include "utils/yaml.h"

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
typedef struct AutomationPoint AutomationPoint;
typedef jack_nframes_t nframes_t;
typedef struct Chord Chord;
typedef struct MusicalScale MusicalScale;

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

  /** Track name, used in channel too. */
  char *              name;

  /**
   * Track Widget created dynamically.
   * 1 track has 1 widget.
   */
  TrackWidget *       widget;
  int                 bot_paned_visible; ///< flag to set automations visible or not
  int                 visible;
  int                 selected;
  int                 handle_pos; ///< position of multipane handle
  int                 mute; ///< muted or not
  int                 solo; ///< solo or not
  int                 recording; ///< recording or not

  /**
   * Track color.
   *
   * This is used in the channels as well.
   */
  GdkRGBA              color;

  /* ==== INSTRUMENT & AUDIO TRACK ==== */
  /**
   * Regions in this track.
   */
  int                   region_ids[MAX_REGIONS];
  Region *              regions[MAX_REGIONS];
  int                   num_regions;  ///< counter

  /* ==== INSTRUMENT & AUDIO TRACK END ==== */

  /* ==== CHORD TRACK ==== */
  MusicalScale *          scale;
  int                     chord_ids[600];
  Chord *                 chords[600];
  int                     num_chords;
  /* ==== CHORD TRACK END ==== */

  /* ==== CHANNEL TRACK ==== */
  /**
   * Owner channel.
   *
   * 1 channel has 1 track.
   */
  int                   channel_id;
  Channel *             channel; ///< cache
  /* ==== CHANNEL TRACK END ==== */

  AutomationTracklist   automation_tracklist;

} Track;

static const cyaml_strval_t
track_type_strings[] =
{
	{ "Instrument",     TRACK_TYPE_INSTRUMENT    },
	{ "Audio",          TRACK_TYPE_AUDIO   },
	{ "Master",         TRACK_TYPE_MASTER   },
	{ "Chord",          TRACK_TYPE_CHORD   },
	{ "Bus",            TRACK_TYPE_BUS   },
};

static const cyaml_schema_field_t
track_fields_schema[] =
{
	CYAML_FIELD_INT (
    "id", CYAML_FLAG_DEFAULT,
    Track, id),
  CYAML_FIELD_STRING_PTR (
    "name", CYAML_FLAG_POINTER,
    Track, name,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    Track, type, track_type_strings,
    CYAML_ARRAY_LEN (track_type_strings)),
	CYAML_FIELD_INT (
    "bot_paned_visible", CYAML_FLAG_DEFAULT,
    Track, bot_paned_visible),
	CYAML_FIELD_INT (
    "visible", CYAML_FLAG_DEFAULT,
    Track, visible),
	CYAML_FIELD_INT (
    "selected", CYAML_FLAG_DEFAULT,
    Track, selected),
	CYAML_FIELD_INT (
    "handle_pos", CYAML_FLAG_DEFAULT,
    Track, handle_pos),
	CYAML_FIELD_INT (
    "mute", CYAML_FLAG_DEFAULT,
    Track, mute),
	CYAML_FIELD_INT (
    "solo", CYAML_FLAG_DEFAULT,
    Track, solo),
	CYAML_FIELD_INT (
    "recording", CYAML_FLAG_DEFAULT,
    Track, recording),
  CYAML_FIELD_MAPPING (
    "color", CYAML_FLAG_DEFAULT,
    Track, color, gdk_rgba_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "region_ids", CYAML_FLAG_DEFAULT,
    Track, region_ids, num_regions,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING_PTR (
    "scale", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
    Track, scale,
    musical_scale_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_ids", CYAML_FLAG_DEFAULT,
    Track, chord_ids, num_chords,
    &int_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "channel_id", CYAML_FLAG_DEFAULT,
    Track, channel_id),
  CYAML_FIELD_MAPPING (
    "automation_tracklist", CYAML_FLAG_DEFAULT,
    Track, automation_tracklist,
    automation_tracklist_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
track_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Track, track_fields_schema),
};

void
track_init_loaded (Track * track);

/**
 * Only to be used by implementing structs.
 *
 * Sets member variables to default values.
 */
void
track_init (Track * track);

/**
 * Returns a new track for the given channel with
 * the given label.
 */
Track *
track_new (Channel * channel, char * label);

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
track_set_muted (Track * track,
                 int     mute,
                 int     trigger_undo);

/**
 * Sets recording and connects/disconnects the
 * JACK ports.
 */
void
track_set_recording (Track *   self,
                     int       recording);

/**
 * Sets track soloed and optionally adds the action
 * to the undo stack.
 */
void
track_set_soloed (Track * track,
                  int     solo,
                  int     trigger_undo);

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
                     int      free);

/**
 * Returns the region at the given position, or NULL.
 */
Region *
track_get_region_at_pos (
  Track *    track,
  Position * pos);

/**
 * Returns the last region in the track, or NULL.
 */
Region *
track_get_last_region (
  Track * track);

/**
 * Returns the last region in the track, or NULL.
 */
AutomationPoint *
track_get_last_automation_point (
  Track * track);

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
