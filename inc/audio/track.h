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
#include "audio/channel.h"
#include "audio/region.h"
#include "audio/scale.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

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
typedef struct ZChord ZChord;
typedef struct MusicalScale MusicalScale;

typedef enum TrackType
{
  /**
   * Instrument tracks must have an Instrument
   * plugin at the first slot and they produce
   * audio output.
   */
  TRACK_TYPE_INSTRUMENT,

  /**
   * Audio tracks can record and contain audio
   * clips. Other than that their channel strips
   * are similar to buses.
   */
  TRACK_TYPE_AUDIO,

  /**
   * The master track is a special type of group
   * track.
   */
  TRACK_TYPE_MASTER,

  /**
   * The chord track contains chords that can be
   * used to modify midi in real time or to color
   * the piano roll.
   */
  TRACK_TYPE_CHORD,

  /**
   * Buses are channels that receive audio input
   * and have effects on their channel strip. They
   * are similar to Group Tracks, except that they
   * cannot be routed to directly. Buses are used
   * for send effects.
   */
  TRACK_TYPE_BUS,

  /**
   * Group Tracks are used for grouping audio
   * signals, for example routing multiple drum
   * tracks to a "Drums" group track. Like buses,
   * they only contain effects but unlike buses
   * they can be routed to.
   */
  TRACK_TYPE_GROUP,

  /**
   * Midi tracks can only have MIDI effects in the
   * strip and produce MIDI output that can be
   * routed to instrument channels or hardware.
   */
  TRACK_TYPE_MIDI,
} TrackType;

/**
 * Track to be inserted into the Project's
 * Tracklist.
 *
 * Each Track contains a Channel with Plugins.
 *
 * Tracks shall be identified by ther position
 * (index) in the Tracklist.
 */
typedef struct Track
{
  /**
   * Position in the Tracklist.
   *
   * This is also used in the Mixer for the Channels.
   * If a track doesn't have a Channel, the Mixer
   * can just skip.
   */
  int                 pos;

  /** The type of track this is. */
  TrackType           type;

  /** Track name, used in channel too. */
  char *              name;

  /**
   * Track Widget created dynamically.
   * 1 track has 1 widget.
   */
  TrackWidget *       widget;

  /** Flag to set automations visible or not. */
  int                 bot_paned_visible;
  int                 visible;
  int                 handle_pos; ///< position of multipane handle
  int                 mute; ///< muted or not
  int                 solo; ///< solo or not

  /** Recording or not. */
  int                 recording;

  /**
   * Active (enabled) or not.
   *
   * Disabled tracks should be ignored in routing.
   */
  int                 active;

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
  Region *              regions[MAX_REGIONS];
  int                   num_regions;  ///< counter

  /* ==== INSTRUMENT & AUDIO TRACK END ==== */

  /* ==== CHORD TRACK ==== */
  MusicalScale *          scale;
  ZChord *                 chords[600];
  int                     num_chords;
  /* ==== CHORD TRACK END ==== */

  /* ==== CHANNEL TRACK ==== */
  /**
   * Owner channel.
   *
   * 1 channel has 1 track.
   */
  Channel *             channel;
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
    "regions", CYAML_FLAG_DEFAULT,
    Track, regions, num_regions,
    &region_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING_PTR (
    "scale", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
    Track, scale,
    musical_scale_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chords", CYAML_FLAG_DEFAULT,
    Track, chords, num_chords,
    &chord_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING_PTR (
    "channel",
    CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
    Track, channel,
    channel_fields_schema),
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
 * Creates a track with the given label and returns
 * it.
 *
 * If the TrackType is one that needs a Channel,
 * then a Channel is also created for the track.
 */
Track *
track_new (
  TrackType type,
  char * label);

/**
 * Clones the track and returns the clone.
 */
Track *
track_clone (Track * track);

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
 * Returns if Track is in TracklistSelections.
 */
int
track_is_selected (Track * self);

/**
 * Wrapper.
 *
 * @param gen_name Generate a unique region name or
 *   not. This will be 0 if the caller already
 *   generated a unique name.
 */
void
track_add_region (
  Track * track,
  Region * region,
  int      gen_name);

/**
 * Only removes the region from the track.
 *
 * Does not free the Region.
 */
void
track_remove_region (
  Track * track,
  Region * region);

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

char *
track_stringize_type (
  TrackType type);

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track);

#endif // __AUDIO_TRACK_H__
