/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
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
#include "audio/modulator_macro_processor.h"
#include "audio/marker.h"
#include "audio/region.h"
#include "audio/scale.h"
#include "audio/scale_object.h"
#include "audio/track_lane.h"
#include "audio/track_processor.h"
#include "plugins/plugin.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

#define MAX_REGIONS 300

typedef struct AutomationTracklist
  AutomationTracklist;
typedef struct ZRegion ZRegion;
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
typedef struct PluginDescriptor PluginDescriptor;
typedef enum PassthroughProcessorType
  PassthroughProcessorType;
typedef enum FaderType FaderType;
typedef void MIDI_FILE;

/**
 * @addtogroup audio
 *
 * @{
 */

#define TRACK_SCHEMA_VERSION 1

#define TRACK_MIN_HEIGHT 24
#define TRACK_DEF_HEIGHT 48

#define TRACK_MAGIC 21890135
#define IS_TRACK(x) \
  (((Track *) x)->magic == TRACK_MAGIC)
#define IS_TRACK_AND_NONNULL(x) \
  (x && IS_TRACK (x))

/**
 * The Track's type.
 */
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
   * Special track for BPM (tempo) and time
   * signature events.
   */
  TRACK_TYPE_TEMPO,

  /**
   * Special track to contain global Modulator's.
   */
  TRACK_TYPE_MODULATOR,

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

static const cyaml_strval_t
track_type_strings[] =
{
  { __("Instrument"),   TRACK_TYPE_INSTRUMENT    },
  { __("Audio"),        TRACK_TYPE_AUDIO   },
  { __("Master"),       TRACK_TYPE_MASTER   },
  { __("Chord"),        TRACK_TYPE_CHORD   },
  { __("Marker"),       TRACK_TYPE_MARKER   },
  { __("Tempo"),        TRACK_TYPE_TEMPO   },
  { __("Modulator"),    TRACK_TYPE_MODULATOR   },
  { __("Audio FX"),     TRACK_TYPE_AUDIO_BUS   },
  { __("Audio Group"),  TRACK_TYPE_AUDIO_GROUP   },
  { __("MIDI"),         TRACK_TYPE_MIDI   },
  { __("MIDI FX"),      TRACK_TYPE_MIDI_BUS   },
  { __("MIDI Group"),   TRACK_TYPE_MIDI_GROUP   },
};

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
  int                 schema_version;

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
  //int                 pos_before_pinned;

  /** The type of track this is. */
  TrackType           type;

  /** Track name, used in channel too. */
  char *              name;

  /** Icon name of the track. */
  char *              icon_name;

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
  //bool                pinned;

  /** Flag to set automations visible or not. */
  bool                automation_visible;

  /** Flag to set track lanes visible or not. */
  bool                lanes_visible;

  /** Whole Track is visible or not. */
  bool                visible;

  /** Height of the main part (without lanes). */
  double              main_height;

  /** Recording or not. */
  bool                recording;

  /**
   * Active (enabled) or not.
   *
   * Disabled tracks should be ignored in routing.
   *
   * TODO explain what functionality this provides.
   */
  bool                active;

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
  size_t              lanes_size;

  /** MIDI channel (MIDI/Instrument track only). */
  uint8_t             midi_ch;

  /**
   * If set to 1, the input received will not be
   * changed to the selected MIDI channel.
   *
   * If this is 0, all input received will have its
   * channel changed to the selected MIDI channel.
   */
  int                 passthrough_midi_input;

  /**
   * ZRegion currently recording on.
   *
   * This must only be set by the RecordingManager
   * when processing an event and should not
   * be touched by anything else.
   */
  ZRegion *           recording_region;

  /**
   * This is a flag to let the recording manager
   * know that a START signal was already sent for
   * recording.
   *
   * This is because \ref Track.recording_region
   * takes a cycle or 2 to become non-NULL.
   */
  bool                recording_start_sent;

  /**
   * This is a flag to let the recording manager
   * know that a STOP signal was already sent for
   * recording.
   *
   * This is because \ref Track.recording_region
   * takes a cycle or 2 to become NULL.
   */
  bool                recording_stop_sent;

  /**
   * This must only be set by the RecordingManager
   * when temporarily pausing recording, eg when
   * looping or leaving the punch range.
   *
   * See \ref
   * RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING.
   */
  bool                recording_paused;

  /** Lane index of region before recording
   * paused. */
  int                 last_lane_idx;

  /* ==== INSTRUMENT/MIDI/AUDIO TRACK END ==== */

  /* ==== AUDIO TRACK ==== */

  /** Real-time time stretcher. */
  Stretcher *          rt_stretcher;

  /* ==== AUDIO TRACK END ==== */

  /* ==== CHORD TRACK ==== */

  /**
   * ChordObject's.
   *
   * Note: these must always be sorted by Position.
   */
  ZRegion **           chord_regions;
  int                 num_chord_regions;
  size_t              chord_regions_size;

  /**
   * ScaleObject's.
   *
   * Note: these must always be sorted by Position.
   */
  ScaleObject **      scales;
  int                 num_scales;
  size_t              scales_size;

  /* ==== CHORD TRACK END ==== */

  /* ==== MARKER TRACK ==== */

  Marker **           markers;
  int                 num_markers;
  size_t              markers_size;

  /* ==== MARKER TRACK END ==== */

  /* ==== TEMPO TRACK ==== */

  /** Automatable BPM control. */
  Port *              bpm_port;

  /** Automatable beats per bar port. */
  Port *              beats_per_bar_port;

  /** Automatable beat unit port. */
  Port *              beat_unit_port;

  /* ==== TEMPO TRACK END ==== */

  /* ==== MODULATOR TRACK ==== */

  /** Modulators. */
  Plugin **         modulators;
  int               num_modulators;
  size_t            modulators_size;

  /** Modulator macros. */
  ModulatorMacroProcessor * modulator_macros[128];
  int               num_modulator_macros;
  int               num_visible_modulator_macros;

  /* ==== MODULATOR TRACK END ==== */

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
  TrackProcessor *    processor;

  AutomationTracklist automation_tracklist;

  /**
   * Flag to tell the UI that this channel had
   * MIDI activity.
   *
   * When processing this and setting it to 0,
   * the UI should create a separate event using
   * EVENTS_PUSH.
   */
  bool                 trigger_midi_activity;

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

  /** User comments. */
  char *               comment;

  /**
   * Set to ON during bouncing if this
   * track should be included.
   *
   * Only relevant for tracks that output audio.
   */
  bool                 bounce;

  /**
   * Tracks that are routed to this track, if
   * group track.
   *
   * This is used when undoing track deletion.
   */
  int *                children;
  int                  num_children;
  size_t               children_size;

  /** Whether the track is currently frozen. */
  bool                 frozen;

  /** Pool ID of the clip if track is frozen. */
  int                  pool_id;

  int                  magic;

  /** Whether this is a project track (as opposed
   * to a clone used in actions). */
  bool                 is_project;
} Track;

static const cyaml_schema_field_t
track_fields_schema[] =
{
  YAML_FIELD_INT (Track, schema_version),
  YAML_FIELD_STRING_PTR (Track, name),
  YAML_FIELD_STRING_PTR (Track, icon_name),
  YAML_FIELD_ENUM (
    Track, type, track_type_strings),
  YAML_FIELD_INT (Track, pos),
  YAML_FIELD_INT (Track, lanes_visible),
  YAML_FIELD_INT (Track, automation_visible),
  YAML_FIELD_INT (Track, visible),
  YAML_FIELD_FLOAT (Track, main_height),
  YAML_FIELD_INT (Track, passthrough_midi_input),
  YAML_FIELD_INT (Track, recording),
  YAML_FIELD_MAPPING_EMBEDDED (
    Track, color, gdk_rgba_fields_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Track, lanes, track_lane_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Track, chord_regions, region_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Track, scales, scale_object_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Track, markers, marker_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track, channel, channel_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track, bpm_port, port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track, beats_per_bar_port, port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track, beat_unit_port, port_fields_schema),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    Track, modulators, plugin_schema),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Track, modulator_macros,
    modulator_macro_processor_schema),
  YAML_FIELD_INT (
    Track, num_visible_modulator_macros),
  YAML_FIELD_MAPPING_PTR (
    Track, processor,
    track_processor_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Track, automation_tracklist,
    automation_tracklist_fields_schema),
  YAML_FIELD_ENUM (
    Track, in_signal_type, port_type_strings),
  YAML_FIELD_ENUM (
    Track, out_signal_type, port_type_strings),
  YAML_FIELD_UINT (
    Track, midi_ch),
  YAML_FIELD_STRING_PTR (
    Track, comment),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT_PRIMITIVES (
    Track, children, int_schema),
  YAML_FIELD_INT (Track, frozen),
  YAML_FIELD_INT (Track, pool_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
track_schema = {
  YAML_VALUE_PTR (
    Track, track_fields_schema),
};

void
track_init_loaded (
  Track * track,
  bool    project);

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
 * @param pos Position in the Tracklist.
 * @param with_lane Init the Track with a lane.
 */
Track *
track_new (
  TrackType    type,
  int          pos,
  const char * label,
  const int    with_lane);

/**
 * Clones the track and returns the clone.
 *
 * @bool src_is_project Whether \ref track is a
 *   project track.
 */
Track *
track_clone (
  Track * track,
  bool    src_is_project);

/**
 * Returns if the given TrackType is a type of
 * Track that has a Channel.
 */
bool
track_type_has_channel (
  TrackType type);

/**
 * Sets magic on objects recursively.
 */
NONNULL
void
track_set_magic (
  Track * self);

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
NONNULL
void
track_set_muted (
  Track * track,
  bool    mute,
  bool    trigger_undo,
  bool    fire_events);

NONNULL
TrackType
track_get_type_from_plugin_descriptor (
  PluginDescriptor * descr);

/**
 * Returns whether the track type is deletable
 * by the user.
 */
NONNULL
bool
track_type_is_deletable (
  TrackType type);

/**
 * Returns the full visible height (main height +
 * height of all visible automation tracks + height
 * of all visible lanes).
 */
NONNULL
double
track_get_full_visible_height (
  Track * self);

bool
track_multiply_heights (
  Track * self,
  double  multiplier,
  bool    visible_only,
  bool    check_only);

/**
 * Returns if the track is soloed.
 */
NONNULL
bool
track_get_soloed (
  Track * self);

/**
 * Returns whether the track is not soloed on its
 * own but its direct out (or its direct out's direct
 * out, etc.) is soloed.
 */
NONNULL
bool
track_get_implied_soloed (
  Track * self);

/**
 * Returns if the track is muted.
 */
NONNULL
bool
track_get_muted (
  Track * self);

/**
 * Sets recording and connects/disconnects the
 * JACK ports.
 */
NONNULL
void
track_set_recording (
  Track *   track,
  bool      recording,
  bool      fire_events);

/**
 * Sets track soloed and optionally adds the action
 * to the undo stack.
 */
NONNULL
void
track_set_soloed (
  Track * track,
  bool    solo,
  bool    trigger_undo,
  bool    fire_events);

/**
 * Returns if Track is in TracklistSelections.
 */
NONNULL
int
track_is_selected (Track * self);

/**
 * Returns whether the track is pinned.
 */
NONNULL
bool
track_is_pinned (
  Track * self);

/**
 * Adds a ZRegion to the given lane or
 * AutomationTrack of the track.
 *
 * The ZRegion must be the main region (see
 * ArrangerObjectInfo).
 *
 * @param at The AutomationTrack of this ZRegion, if
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
  ZRegion *         region,
  AutomationTrack * at,
  int               lane_pos,
  int               gen_name,
  int               fire_events);

/**
 * Inserts a ZRegion to the given lane or
 * AutomationTrack of the track, at the given
 * index.
 *
 * The ZRegion must be the main region (see
 * ArrangerObjectInfo).
 *
 * @param at The AutomationTrack of this ZRegion, if
 *   automation region.
 * @param lane_pos The position of the lane to add
 *   to, if applicable.
 * @param idx The index to insert the region at
 *   inside its parent, or -1 to append.
 * @param gen_name Generate a unique region name or
 *   not. This will be 0 if the caller already
 *   generated a unique name.
 */
void
track_insert_region (
  Track *           track,
  ZRegion *         region,
  AutomationTrack * at,
  int               lane_pos,
  int               idx,
  int               gen_name,
  int               fire_events);

/**
 * Writes the track to the given MIDI file.
 */
NONNULL
void
track_write_to_midi_file (
  Track *     self,
  MIDI_FILE * mf);

/**
 * Appends the Track to the selections.
 *
 * @param exclusive Select only this track.
 * @param fire_events Fire events to update the
 *   UI.
 */
NONNULL
void
track_select (
  Track * self,
  bool    select,
  bool    exclusive,
  bool    fire_events);

/**
 * Unselects all arranger objects in the track.
 */
NONNULL
void
track_unselect_all (
  Track * self);

/**
 * Removes all objects recursively from the track.
 */
NONNULL
void
track_clear (
  Track * self);

/**
 * Only removes the region from the track.
 *
 * @pararm free Also free the Region.
 */
NONNULL
void
track_remove_region (
  Track *   self,
  ZRegion * region,
  bool      fire_events,
  bool      free);

/**
 * Wrapper for audio and MIDI/instrument tracks
 * to fill in MidiEvents or StereoPorts from the
 * timeline data.
 *
 * @note The engine splits the cycle so transport
 *   loop related logic is not needed.
 *
 * @param g_start_frame Global start frame.
 * @param local_start_frame The start frame offset
 *   from 0 in this cycle.
 * @param nframes Number of frames at start
 *   Position.
 * @param stereo_ports StereoPorts to fill.
 * @param midi_events MidiEvents to fill (from
 *   Piano Roll Port for example).
 */
void
track_fill_events (
  Track *         track,
  const long      g_start_frames,
  const nframes_t local_start_frame,
  nframes_t       nframes,
  MidiEvents *    midi_events,
  StereoPorts *   stereo_ports);

/**
 * Verifies the identifiers on a live Track
 * (in the project, not a clone).
 *
 * @return True if pass.
 */
bool
track_validate (
  Track * self);

/**
 * Returns the region at the given position, or
 * NULL.
 *
 * @param include_region_end Whether to include the
 *   region's end in the calculation.
 */
ZRegion *
track_get_region_at_pos (
  const Track *    track,
  const Position * pos,
  bool             include_region_end);

/**
 * Returns the last ZRegion in the
 * track, or NULL.
 */
ZRegion *
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
  Track *    track,
  const bool visible);

/**
 * Returns if the given TrackType can host the
 * given RegionType.
 */
int
track_type_can_host_region_type (
  const TrackType  tt,
  const RegionType rt);

static inline bool
track_type_has_mono_compat_switch (
  const TrackType tt)
{
  return
    tt == TRACK_TYPE_AUDIO_GROUP ||
    tt == TRACK_TYPE_MASTER;
}

#define track_type_is_audio_group \
  track_type_has_mono_compat_switch

static inline bool
track_type_is_fx (
  const TrackType type)
{
  return
    type == TRACK_TYPE_AUDIO_BUS ||
    type == TRACK_TYPE_MIDI_BUS;
}

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
    type == TRACK_TYPE_INSTRUMENT ||
    type == TRACK_TYPE_CHORD;
}

/**
 * Generates automatables for the track.
 *
 * Should be called as soon as the track is
 * created.
 */
void
track_generate_automation_tracks (
  Track * track);

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
 * Updates the track's children.
 *
 * Used when changing track positions.
 */
void
track_update_children (
  Track * self);

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

Track *
track_find_by_name (
  const char * name);

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
  size_t *         velocities_size,
  int              inside);

/**
 * Getter for the track name.
 */
const char *
track_get_name (Track * track);

/**
 * Setter to be used by the UI to create an
 * undoable action.
 */
void
track_set_name_with_action (
  Track *      track,
  const char * name);

/**
 * Setter for the track name.
 *
 * If a track with that name already exists, it
 * adds a number at the end.
 *
 * Must only be called from the GTK thread.
 */
void
track_set_name (
  Track *      track,
  const char * _name,
  bool         pub_events);

/**
 * Returns the Track from the Project matching
 * \p name.
 *
 * @param name Name to search for.
 */
Track *
track_get_from_name (
  const char * name);

const char *
track_stringize_type (
  TrackType type);

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
 * Returns the Fader (if applicable).
 *
 * @param post_fader True to get post fader,
 *   false to get pre fader.
 */
Fader *
track_get_fader (
  Track * track,
  bool    post_fader);

/**
 * Returns the FaderType corresponding to the given
 * Track.
 */
FaderType
track_get_fader_type (
  const Track * track);

/**
 * Returns the prefader type
 * corresponding to the given Track.
 */
FaderType
track_type_get_prefader_type (
  TrackType type);

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
 * Returns all the regions inside the given range,
 * or all the regions if both @ref p1 and @ref p2
 * are NULL.
 *
 * @return The number of regions returned.
 */
int
track_get_regions_in_range (
  Track *    self,
  Position * p1,
  Position * p2,
  ZRegion ** regions);

void
track_activate_all_plugins (
  Track * track,
  bool    activate);

/**
 * Comment setter.
 *
 * @note This creates an undoable action.
 */
void
track_comment_setter (
  void *        track,
  const char *  comment);

/**
 * @param undoable Create an undable action.
 */
void
track_set_comment (
  Track *       self,
  const char *  comment,
  bool          undoable);

/**
 * Comment getter.
 */
const char *
track_get_comment (
  void *  track);

/**
 * Sets the track color.
 */
void
track_set_color (
  Track *         self,
  const GdkRGBA * color,
  bool            undoable,
  bool            fire_events);

/**
 * Sets the track icon.
 */
void
track_set_icon (
  Track *      self,
  const char * icon_name,
  bool         undoable,
  bool         fire_events);

/**
 * Recursively marks the track and children as
 * project objects or not.
 */
void
track_set_is_project (
  Track * self,
  bool    is_project);

/**
 * Returns the plugin at the given slot, if any.
 *
 * @param slot The slot (ignored if instrument is
 *   selected.
 */
Plugin *
track_get_plugin_at_slot (
  Track *           track,
  PluginSlotType    slot_type,
  int               slot);

/**
 * Marks the track for bouncing.
 *
 * @param mark_children Whether to mark all
 *   children tracks as well. Used when exporting
 *   stems on the specific track stem only.
 */
void
track_mark_for_bounce (
  Track * self,
  bool    bounce,
  bool    mark_regions,
  bool    mark_children);

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 *
 * @param size Current array count.
 * @param is_dynamic Whether the array can be
 *   dynamically resized.
 * @param max_size Current array size, if dynamic.
 */
void
track_append_all_ports (
  Track *   self,
  Port ***  ports,
  int *     size,
  bool      is_dynamic,
  size_t *  max_size,
  bool      include_plugins);

/**
 * Freezes or unfreezes the track.
 *
 * When a track is frozen, it is bounced with
 * effects to a temporary file in the pool, which
 * is played back directly from disk.
 *
 * When the track is unfrozen, this file will be
 * removed from the pool and the track will be
 * played normally again.
 */
void
track_freeze (
  Track * self,
  bool    freeze);

/**
 * Wrapper over channel_add_plugin() and
 * modulator_track_insert_modulator().
 */
void
track_insert_plugin (
  Track *        self,
  Plugin *       pl,
  PluginSlotType slot_type,
  int            slot,
  bool           replacing_plugin,
  bool           moving_plugin,
  bool           confirm,
  bool           gen_automatables,
  bool           recalc_graph,
  bool           fire_events);

/**
 * Wrapper over channel_add_plugin() and
 * modulator_track_insert_modulator().
 */
void
track_add_plugin (
  Track *        self,
  PluginSlotType slot_type,
  int            slot,
  Plugin *       pl,
  bool           replacing_plugin,
  bool           moving_plugin,
  bool           gen_automatables,
  bool           recalc_graph,
  bool           fire_events);

/**
 * Wrapper over channel_remove_plugin() and
 * modulator_track_remove_modulator().
 */
void
track_remove_plugin (
  Track *        self,
  PluginSlotType slot_type,
  int            slot,
  bool           replacing_plugin,
  bool           moving_plugin,
  bool           deleting_plugin,
  bool           deleting_track,
  bool           recalc_graph);

/**
 * Disconnects the track from the processing
 * chain.
 *
 * This should be called immediately when the
 * track is getting deleted, and track_free
 * should be designed to be called later after
 * an arbitrary delay.
 *
 * @param remove_pl Remove the Plugin from the
 *   Channel. Useful when deleting the channel.
 * @param recalc_graph Recalculate mixer graph.
 */
void
track_disconnect (
  Track * self,
  bool    remove_pl,
  bool    recalc_graph);

static inline const char *
track_type_to_string (
  TrackType type)
{
  return track_type_strings[type].str;
}

TrackType
track_type_get_from_string (
  const char * str);

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track);

/**
 * @}
 */

#endif // __AUDIO_TRACK_H__
