/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
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
#include "audio/chord_object.h"
#include "audio/marker.h"
#include "audio/region.h"
#include "audio/scale.h"
#include "audio/scale_object.h"
#include "audio/track_lane.h"
#include "audio/track_processor.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

#define MAX_REGIONS 300
#define MAX_MODULATORS 14

typedef struct AutomationTracklist
  AutomationTracklist;
typedef struct Region Region;
typedef struct Position Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct MidiEvents MidiEvents;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;
typedef struct AutomationPoint AutomationPoint;
typedef struct ChordObject ChordObject;
typedef struct MusicalScale MusicalScale;
typedef struct Modulator Modulator;
typedef struct Marker Marker;
typedef enum PassthroughProcessorType
  PassthroughProcessorType;
typedef enum FaderType FaderType;
typedef void MIDI_FILE;

/**
 * @addtogroup audio
 *
 * @{
 */

#define TRACK_MIN_HEIGHT 24
#define TRACK_DEF_HEIGHT 48

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
   * Marker Track's contain named markers at
   * specific Position's in the song.
   */
  TRACK_TYPE_MARKER,

  /**
   * Buses are channels that receive audio input
   * and have effects on their channel strip. They
   * are similar to Group Tracks, except that they
   * cannot be routed to directly. Buses are used
   * for send effects.
   */
  TRACK_TYPE_AUDIO_BUS,

  /**
   * Group Tracks are used for grouping audio
   * signals, for example routing multiple drum
   * tracks to a "Drums" group track. Like buses,
   * they only contain effects but unlike buses
   * they can be routed to.
   */
  TRACK_TYPE_AUDIO_GROUP,

  /**
   * Midi tracks can only have MIDI effects in the
   * strip and produce MIDI output that can be
   * routed to instrument channels or hardware.
   */
  TRACK_TYPE_MIDI,


  /** Same with audio bus but for MIDI signals. */
  TRACK_TYPE_MIDI_BUS,

  /** Same with audio group but for MIDI signals. */
  TRACK_TYPE_MIDI_GROUP,
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

  /**
   * Used to remember the position before pinned,
   * so it can be moved back there when unpinned.
   */
  int                 pos_before_pinned;

  /** The type of track this is. */
  TrackType           type;

  /** Track name, used in channel too. */
  char *              name;

  /**
   * Track Widget created dynamically.
   * 1 track has 1 widget.
   */
  TrackWidget *       widget;

  /**
   * Whether pinned or not.
   *
   * Pinned tracks should keep their original pos
   * saved so they can get unpinned. When iterating
   * through unpinned tracks, can just check this
   * variable.
   */
  int                 pinned;

  /** Flag to set automations visible or not. */
  int                 automation_visible;

  /** Flag to set track lanes visible or not. */
  int                 lanes_visible;

  /** Whole Track is visible or not. */
  int                 visible;

  /** Height of the main part (without lanes). */
  int                 main_height;

  /** Muted or not. */
  int                 mute;

  /** Soloed or not. */
  int                 solo;

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
  GdkRGBA             color;

  /* ==== INSTRUMENT/MIDI/AUDIO TRACK ==== */

  /** Lanes in this track containing Regions. */
  TrackLane **        lanes;
  int                 num_lanes;
  int                 lanes_size;

  /** MIDI channel (MIDI/Instrument track only). */
  uint8_t             midi_ch;

  /**
   * Region currently recording on.
   *
   * This must only be set by the RecordingManager
   * and should not be touched by anything else.
   */
  Region *            recording_region;

  /* ==== INSTRUMENT/MIDI/AUDIO TRACK END ==== */

  /* ==== CHORD TRACK ==== */

  /**
   * ChordObject's.
   *
   * Note: these must always be sorted by Position.
   */
  Region **           chord_regions;
  int                 num_chord_regions;
  int                 chord_regions_size;

  /**
   * ScaleObject's.
   *
   * Note: these must always be sorted by Position.
   */
  ScaleObject **      scales;
  int                 num_scales;
  int                 scales_size;

  /* ==== CHORD TRACK END ==== */

  /* ==== MARKER TRACK ==== */

  Marker **           markers;
  int                 num_markers;
  int                 markers_size;

  /* ==== MARKER TRACK END ==== */

  /* ==== CHANNEL TRACK ==== */

  /** 1 Track has 0 or 1 Channel. */
  Channel *           channel;

  /* ==== CHANNEL TRACK END ==== */

  /**
   * The TrackProcessor, used for processing.
   *
   * This is the starting point when processing
   * a Track.
   */
  TrackProcessor      processor;

  AutomationTracklist automation_tracklist;

  /** Modulators for this Track. */
  Modulator **        modulators;
  int                 num_modulators;
  int                 modulators_size;

  /**
   * Flag to tell the UI that this channel had
   * MIDI activity.
   *
   * When processing this and setting it to 0,
   * the UI should create a separate event using
   * EVENTS_PUSH.
   */
  int                  trigger_midi_activity;

  /**
   * The input signal type (eg audio bus tracks have
   * audio input signals).
   */
  PortType             in_signal_type;

  /**
   * The output signal type (eg midi tracks have
   * MIDI output singals).
   */
  PortType             out_signal_type;

} Track;

static const cyaml_strval_t
track_type_strings[] =
{
  { "Instrument",     TRACK_TYPE_INSTRUMENT    },
  { "Audio",          TRACK_TYPE_AUDIO   },
  { "MIDI",           TRACK_TYPE_MIDI   },
  { "Master",         TRACK_TYPE_MASTER   },
  { "Chord",          TRACK_TYPE_CHORD   },
  { "Audio Bus",      TRACK_TYPE_AUDIO_BUS   },
  { "MIDI Bus",       TRACK_TYPE_MIDI_BUS   },
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
    "pos", CYAML_FLAG_DEFAULT,
    Track, pos),
  CYAML_FIELD_INT (
    "pos_before_pinned", CYAML_FLAG_DEFAULT,
    Track, pos_before_pinned),
  CYAML_FIELD_INT (
    "lanes_visible", CYAML_FLAG_DEFAULT,
    Track, lanes_visible),
  CYAML_FIELD_INT (
    "automation_visible", CYAML_FLAG_DEFAULT,
    Track, automation_visible),
  CYAML_FIELD_INT (
    "visible", CYAML_FLAG_DEFAULT,
    Track, visible),
  CYAML_FIELD_INT (
    "main_height", CYAML_FLAG_DEFAULT,
    Track, main_height),
  CYAML_FIELD_INT (
    "mute", CYAML_FLAG_DEFAULT,
    Track, mute),
  CYAML_FIELD_INT (
    "solo", CYAML_FLAG_DEFAULT,
    Track, solo),
  CYAML_FIELD_INT (
    "recording", CYAML_FLAG_DEFAULT,
    Track, recording),
  CYAML_FIELD_INT (
    "pinned", CYAML_FLAG_DEFAULT,
    Track, pinned),
  CYAML_FIELD_MAPPING (
    "color", CYAML_FLAG_DEFAULT,
    Track, color, gdk_rgba_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "lanes", CYAML_FLAG_POINTER,
    Track, lanes, num_lanes,
    &track_lane_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_regions", CYAML_FLAG_POINTER,
    Track, chord_regions, num_chord_regions,
    &region_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "scales", CYAML_FLAG_POINTER,
    Track, scales, num_scales,
    &scale_object_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "markers", CYAML_FLAG_POINTER,
    Track, markers, num_markers,
    &marker_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING_PTR (
    "channel",
    CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
    Track, channel,
    channel_fields_schema),
  CYAML_FIELD_MAPPING (
    "processor", CYAML_FLAG_DEFAULT,
    Track, processor,
    track_processor_fields_schema),
  CYAML_FIELD_MAPPING (
    "automation_tracklist", CYAML_FLAG_DEFAULT,
    Track, automation_tracklist,
    automation_tracklist_fields_schema),
  CYAML_FIELD_ENUM (
    "in_signal_type", CYAML_FLAG_DEFAULT,
    Track, in_signal_type, port_type_strings,
    CYAML_ARRAY_LEN (port_type_strings)),
  CYAML_FIELD_ENUM (
    "out_signal_type", CYAML_FLAG_DEFAULT,
    Track, out_signal_type, port_type_strings,
    CYAML_ARRAY_LEN (port_type_strings)),
  CYAML_FIELD_UINT (
    "midi_ch", CYAML_FLAG_DEFAULT,
    Track, midi_ch),

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
 * Inits the Track, optionally adding a single
 * lane.
 *
 * @param add_lane Add a lane. This should be used
 *   for new Tracks. When cloning, the lanes should
 *   be cloned so this should be 0.
 */
void
track_init (
  Track *   self,
  const int add_lane);

/**
 * Creates a track with the given label and returns
 * it.
 *
 * If the TrackType is one that needs a Channel,
 * then a Channel is also created for the track.
 *
 * @param with_lane Init the Track with a lane.
 */
Track *
track_new (
  TrackType type,
  char * label,
  const int with_lane);

/**
 * Clones the track and returns the clone.
 */
Track *
track_clone (Track * track);

/**
 * Returns if the given TrackType is a type of
 * Track that has a Channel.
 */
static inline int
track_type_has_channel (
  TrackType type)
{
  if (type == TRACK_TYPE_CHORD ||
      type == TRACK_TYPE_MARKER)
    return 0;

  return 1;
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
track_set_muted (Track * track,
                 int     mute,
                 int     trigger_undo);

/**
 * Returns the full visible height (main height +
 * height of all visible automation tracks + height
 * of all visible lanes).
 */
int
track_get_full_visible_height (
  Track * self);

/**
 * Sets recording and connects/disconnects the
 * JACK ports.
 */
void
track_set_recording (
  Track *   track,
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
 * Adds a Region to the given lane or
 * AutomationTrack of the track.
 *
 * The Region must be the main region (see
 * ArrangerObjectInfo).
 *
 * @param at The AutomationTrack of this Region, if
 *   automation region.
 * @param lane_pos The position of the lane to add
 *   to, if applicable.
 * @param gen_name Generate a unique region name or
 *   not. This will be 0 if the caller already
 *   generated a unique name.
 */
void
track_add_region (
  Track *           track,
  Region *          region,
  AutomationTrack * at,
  int               lane_pos,
  int               gen_name,
  int               fire_events);

/**
 * Writes the track to the given MIDI file.
 */
void
track_write_to_midi_file (
  const Track * self,
  MIDI_FILE *   mf);

/**
 * Appends the Track to the selections.
 *
 * @param exclusive Select only this track.
 * @param fire_events Fire events to update the
 *   UI.
 */
void
track_select (
  Track * self,
  int     select,
  int     exclusive,
  int     fire_events);

/**
 * Removes the region from the track.
 *
 * @pararm free Also free the Region.
 */
void
track_remove_region (
  Track *  track,
  Region * region,
  int      fire_events,
  int      free);

/**
 * Returns the region at the given position, or NULL.
 */
Region *
track_get_region_at_pos (
  const Track *    track,
  const Position * pos);

/**
 * Returns the last Region in the
 * track, or NULL.
 */
Region *
track_get_last_region (
  Track *    track);

/**
 * Set track lanes visible and fire events.
 */
void
track_set_lanes_visible (
  Track *   track,
  const int visible);

/**
 * Set automation visible and fire events.
 */
void
track_set_automation_visible (
  Track *   track,
  const int visible);

/**
 * Returns if the given TrackType can host the
 * given RegionType.
 */
int
track_type_can_host_region_type (
  const TrackType  tt,
  const RegionType rt);

/**
 * Returns if the Track can record.
 */
static inline int
track_type_can_record (
  const TrackType type)
{
  return
    type == TRACK_TYPE_AUDIO ||
    type == TRACK_TYPE_MIDI ||
    type == TRACK_TYPE_INSTRUMENT;
}

/**
 * Wrapper.
 */
void
track_setup (Track * track);

/**
 * Returns the automation tracklist if the track
 * type has one,
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
 * Updates position in the tracklist and also
 * updates the information in the lanes.
 */
void
track_set_pos (
  Track * track,
  int     pos);

/**
 * Returns if the Track should have a piano roll.
 */
int
track_has_piano_roll (
  const Track * track);

/**
 * Returns if the Track should have an inputs
 * selector.
 */
static inline int
track_has_inputs (
  const Track * track)
{
  return
    track->type == TRACK_TYPE_MIDI ||
    track->type == TRACK_TYPE_INSTRUMENT ||
    track->type == TRACK_TYPE_AUDIO;
}

/**
 * Fills in the array with all the velocities in
 * the project that are within or outside the
 * range given.
 *
 * @param inside Whether to find velocities inside
 *   the range (1) or outside (0).
 */
void
track_get_velocities_in_range (
  const Track *    track,
  const Position * start_pos,
  const Position * end_pos,
  Velocity ***     velocities,
  int *            num_velocities,
  int *            velocities_size,
  int              inside);

/**
 * Getter for the track name.
 */
const char *
track_get_name (Track * track);

/**
 * Setter for the track name.
 *
 * If the track name is duplicate, it discards the
 * new name.
 */
void
track_set_name (
  Track * track,
  const char *  name);

/**
 * Returns the Track from the Project matching
 * \p name.
 *
 * @param name Name to search for.
 */
Track *
track_get_from_name (
  const char * name);

char *
track_stringize_type (
  TrackType type);

/**
 * Adds and connects a Modulator to the Track.
 */
void
track_add_modulator (
  Track * track,
  Modulator * modulator);

/**
 * Returns if regions in tracks from type1 can
 * be moved to type2.
 */
static inline int
track_type_is_compatible_for_moving (
  const TrackType type1,
  const TrackType type2)
{
  return
    type1 == type2 ||
    (type1 == TRACK_TYPE_MIDI &&
     type2 == TRACK_TYPE_INSTRUMENT) ||
    (type1 == TRACK_TYPE_INSTRUMENT &&
     type2 == TRACK_TYPE_MIDI);
}

/**
 * Updates the frames of each position in each child
 * of the track recursively.
 */
void
track_update_frames (
  Track * track);

/**
 * Returns the FaderType corresponding to the given
 * Track.
 */
FaderType
track_get_fader_type (
  const Track * track);

/**
 * Returns the PassthroughProcessorType
 * corresponding to the given Track.
 */
PassthroughProcessorType
track_get_passthrough_processor_type (
  const Track * track);

/**
 * Creates missing TrackLane's until pos.
 */
void
track_create_missing_lanes (
  Track *   track,
  const int pos);

/**
 * Removes the empty last lanes of the Track
 * (except the last one).
 */
void
track_remove_empty_last_lanes (
  Track * track);

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track);

/**
 * @}
 */

#endif // __AUDIO_TRACK_H__
