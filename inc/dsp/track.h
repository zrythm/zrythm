// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * The backend for a timeline track.
 */

#ifndef __AUDIO_TRACK_H__
#define __AUDIO_TRACK_H__

#include "dsp/automation_tracklist.h"
#include "dsp/channel.h"
#include "dsp/chord_object.h"
#include "dsp/marker.h"
#include "dsp/modulator_macro_processor.h"
#include "dsp/region.h"
#include "dsp/scale.h"
#include "dsp/scale_object.h"
#include "dsp/track_lane.h"
#include "dsp/track_processor.h"
#include "plugins/plugin.h"
#include "utils/yaml.h"

#include <glib/gi18n.h>

typedef struct AutomationTracklist  AutomationTracklist;
typedef struct ZRegion              ZRegion;
typedef struct Position             Position;
typedef struct _TrackWidget         TrackWidget;
typedef struct _FolderChannelWidget FolderChannelWidget;
typedef struct Channel              Channel;
typedef struct MidiEvents           MidiEvents;
typedef struct AutomationTrack      AutomationTrack;
typedef struct Automatable          Automatable;
typedef struct AutomationPoint      AutomationPoint;
typedef struct ChordObject          ChordObject;
typedef struct MusicalScale         MusicalScale;
typedef struct Modulator            Modulator;
typedef struct Marker               Marker;
typedef struct PluginDescriptor     PluginDescriptor;
typedef struct Tracklist            Tracklist;
typedef struct SupportedFile        SupportedFile;
typedef struct TracklistSelections  TracklistSelections;
enum class PassthroughProcessorType;
enum class FaderType;
typedef void                                  MIDI_FILE;
typedef struct _WrappedObjectWithChangeSignal WrappedObjectWithChangeSignal;

TYPEDEF_STRUCT_UNDERSCORED (FileImportInfo);

/**
 * @addtogroup dsp
 *
 * @{
 */

#define TRACK_MIN_HEIGHT 24
#define TRACK_DEF_HEIGHT 48

#define TRACK_MAGIC 21890135
#define IS_TRACK(x) (((Track *) x)->magic == TRACK_MAGIC)
#define IS_TRACK_AND_NONNULL(x) (x && IS_TRACK (x))

#define TRACK_MAX_MODULATOR_MACROS 128

#define TRACK_DND_PREFIX Z_DND_STRING_PREFIX "Track::"

#define track_is_in_active_project(self) \
  (self->tracklist && tracklist_is_in_active_project (self->tracklist))

/** Whether this track is part of the
 * SampleProcessor auditioner tracklist. */
#define track_is_auditioner(self) \
  (self->tracklist && tracklist_is_auditioner (self->tracklist))

/**
 * The Track's type.
 */
enum class TrackType
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

  /** Foldable track used for visual grouping. */
  TRACK_TYPE_FOLDER,
};

static const char * track_type_strings[] = {
  N_ ("Instrument"),  N_ ("Audio"), N_ ("Master"),    N_ ("Chord"),
  N_ ("Marker"),      N_ ("Tempo"), N_ ("Modulator"), N_ ("Audio FX"),
  N_ ("Audio Group"), N_ ("MIDI"),  N_ ("MIDI FX"),   N_ ("MIDI Group"),
  N_ ("Folder"),
};

/**
 * Track to be inserted into the Project's Tracklist.
 *
 * Each Track contains a Channel with Plugins.
 *
 * Tracks shall be identified by their position (index) in the Tracklist.
 */
typedef struct Track
{
  /**
   * Position in the Tracklist.
   *
   * This is also used in the Mixer for the Channels. If a track doesn't have
   * a Channel, the Mixer can just skip.
   */
  int pos;

  /** The type of track this is. */
  TrackType type;

  /** Track name, used in channel too. */
  char * name;

  /** Cache calculated when adding to graph. */
  unsigned int name_hash;

  /** Icon name of the track. */
  char * icon_name;

  /**
   * Track Widget created dynamically.
   * 1 track has 1 widget.
   */
  TrackWidget * widget;

  /**
   * Widget used for foldable tracks in the mixer.
   */
  FolderChannelWidget * folder_ch_widget;

  /** Flag to set automations visible or not. */
  bool automation_visible;

  /** Flag to set track lanes visible or not. */
  bool lanes_visible;

  /** Whole Track is visible or not. */
  bool visible;

  /** Track will be hidden if true (temporary and not
   * serializable). */
  bool filtered;

  /** Height of the main part (without lanes). */
  double main_height;

  /** Recording or not. */
  Port * recording;

  /**
   * Whether record was set automatically when
   * the channel was selected.
   *
   * This is so that it can be unset when selecting
   * another track. If we don't do this all the
   * tracks end up staying on record mode.
   */
  bool record_set_automatically;

  /**
   * Active (enabled) or not.
   *
   * Disabled tracks should be ignored in routing.
   * Similar to Plugin.enabled (bypass).
   */
  bool enabled;

  /**
   * Track color.
   *
   * This is used in the channels as well.
   */
  GdkRGBA color;

  /* ==== INSTRUMENT/MIDI/AUDIO TRACK ==== */

  /** Lanes in this track containing Regions. */
  TrackLane ** lanes;
  int          num_lanes;
  size_t       lanes_size;

  /** Snapshots used during playback. */
  TrackLane ** lane_snapshots;
  int          num_lane_snapshots;

  /** MIDI channel (MIDI/Instrument track only). */
  uint8_t midi_ch;

  /**
   * Whether drum mode in the piano roll is
   * enabled for this track.
   *
   * Only used for tracks that have a piano roll.
   */
  bool drum_mode;

  /**
   * If set to 1, the input received will not be
   * changed to the selected MIDI channel.
   *
   * If this is 0, all input received will have its
   * channel changed to the selected MIDI channel.
   */
  int passthrough_midi_input;

  /**
   * ZRegion currently recording on.
   *
   * This must only be set by the RecordingManager
   * when processing an event and should not
   * be touched by anything else.
   */
  ZRegion * recording_region;

  /**
   * This is a flag to let the recording manager
   * know that a START signal was already sent for
   * recording.
   *
   * This is because \ref Track.recording_region
   * takes a cycle or 2 to become non-NULL.
   */
  bool recording_start_sent;

  /**
   * This is a flag to let the recording manager
   * know that a STOP signal was already sent for
   * recording.
   *
   * This is because \ref Track.recording_region
   * takes a cycle or 2 to become NULL.
   */
  bool recording_stop_sent;

  /**
   * This must only be set by the RecordingManager
   * when temporarily pausing recording, eg when
   * looping or leaving the punch range.
   *
   * See \ref
   * RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING.
   */
  bool recording_paused;

  /** Lane index of region before recording
   * paused. */
  int last_lane_idx;

  /* ==== INSTRUMENT/MIDI/AUDIO TRACK END ==== */

  /* ==== AUDIO TRACK ==== */

  /** Real-time time stretcher. */
  Stretcher * rt_stretcher;

  /* ==== AUDIO TRACK END ==== */

  /* ==== CHORD TRACK ==== */

  /**
   * ChordObject's.
   *
   * Note: these must always be sorted by Position.
   */
  ZRegion ** chord_regions;
  int        num_chord_regions;
  size_t     chord_regions_size;

  /** Snapshots used during playback. */
  ZRegion ** chord_region_snapshots;
  int        num_chord_region_snapshots;

  /**
   * ScaleObject's.
   *
   * Note: these must always be sorted by Position.
   */
  ScaleObject ** scales;
  int            num_scales;
  size_t         scales_size;

  /** Snapshots used during playback TODO unimplemented. */
  ScaleObject ** scale_snapshots;
  int            num_scale_snapshots;

  /* ==== CHORD TRACK END ==== */

  /* ==== MARKER TRACK ==== */

  Marker ** markers;
  int       num_markers;
  size_t    markers_size;

  /** Snapshots used during playback TODO unimplemented. */
  Marker ** marker_snapshots;
  int       num_marker_snapshots;

  /* ==== MARKER TRACK END ==== */

  /* ==== TEMPO TRACK ==== */

  /** Automatable BPM control. */
  Port * bpm_port;

  /** Automatable beats per bar port. */
  Port * beats_per_bar_port;

  /** Automatable beat unit port. */
  Port * beat_unit_port;

  /* ==== TEMPO TRACK END ==== */

  /* ==== FOLDABLE TRACK ==== */

  /**
   * Number of tracks inside this track.
   *
   * Should be 1 unless foldable.
   */
  int size;

  /** Whether currently folded. */
  bool folded;

  /* ==== FOLDABLE TRACK END ==== */

  /* ==== MODULATOR TRACK ==== */

  /** Modulators. */
  Plugin ** modulators;
  int       num_modulators;
  size_t    modulators_size;

  /** Modulator macros. */
  ModulatorMacroProcessor * modulator_macros[TRACK_MAX_MODULATOR_MACROS];
  int                       num_modulator_macros;
  int                       num_visible_modulator_macros;

  /* ==== MODULATOR TRACK END ==== */

  /* ==== CHANNEL TRACK ==== */

  /** 1 Track has 0 or 1 Channel. */
  Channel * channel;

  /* ==== CHANNEL TRACK END ==== */

  /**
   * The TrackProcessor, used for processing.
   *
   * This is the starting point when processing
   * a Track.
   */
  TrackProcessor * processor;

  AutomationTracklist automation_tracklist;

  /**
   * Flag to tell the UI that this channel had
   * MIDI activity.
   *
   * When processing this and setting it to 0,
   * the UI should create a separate event using
   * EVENTS_PUSH.
   */
  bool trigger_midi_activity;

  /**
   * The input signal type (eg audio bus tracks have
   * audio input signals).
   */
  ZPortType in_signal_type;

  /**
   * The output signal type (eg midi tracks have
   * MIDI output signals).
   */
  ZPortType out_signal_type;

  /** User comments. */
  char * comment;

  /**
   * Set to ON during bouncing if this
   * track should be included.
   *
   * Only relevant for tracks that output audio.
   */
  bool bounce;

  /**
   * Whether to temporarily route the output to master (e.g.,
   * when bouncing the track on its own without its parents).
   */
  bool bounce_to_master;

  /**
   * Name hashes of tracks that are routed to this track, if
   * group track.
   *
   * This is used when undoing track deletion.
   */
  unsigned int * children;
  int            num_children;
  size_t         children_size;

  /** Whether the track is currently frozen. */
  bool frozen;

  /** Pool ID of the clip if track is frozen. */
  int pool_id;

  int magic;

  /** Whether currently disconnecting. */
  bool disconnecting;

  /** Pointer to owner tracklist, if any. */
  Tracklist * tracklist;

  /** Pointer to owner tracklist selections, if any. */
  TracklistSelections * ts;

  /**
   * Last lane created during this drag.
   *
   * This is used to prevent creating infinite lanes when you
   * want to track a region from the last lane to the track
   * below. Only 1 new lane will be created in case the
   * user wants to move the region to a new lane instead of
   * the track below.
   *
   * Used when moving regions vertically.
   */
  int last_lane_created;

  /** Block auto-creating or deleting lanes. */
  bool block_auto_creation_and_deletion;

  /** Used in Gtk. */
  WrappedObjectWithChangeSignal * gobj;
} Track;

COLD NONNULL_ARGS (1) void track_init_loaded (
  Track *               self,
  Tracklist *           tracklist,
  TracklistSelections * ts);

/**
 * Inits the Track, optionally adding a single
 * lane.
 *
 * @param add_lane Add a lane. This should be used
 *   for new Tracks. When cloning, the lanes should
 *   be cloned so this should be 0.
 */
void
track_init (Track * self, const int add_lane);

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
track_new (TrackType type, int pos, const char * label, const int with_lane);

/**
 * Clones the track and returns the clone.
 *
 * @param error To be filled if an error occurred.
 */
NONNULL_ARGS (1) Track * track_clone (Track * track, GError ** error);

/**
 * Returns if the given TrackType is a type of
 * Track that has a Channel.
 */
bool
track_type_has_channel (TrackType type);

static inline bool
track_type_can_have_direct_out (TrackType type)
{
  return type != TrackType::TRACK_TYPE_MASTER;
}

static inline bool
track_type_can_have_region_type (TrackType type, RegionType region_type)
{
  switch (region_type)
    {
    case RegionType::REGION_TYPE_AUDIO:
      return type == TrackType::TRACK_TYPE_AUDIO;
    case RegionType::REGION_TYPE_MIDI:
      return type == TrackType::TRACK_TYPE_MIDI
             || type == TrackType::TRACK_TYPE_INSTRUMENT;
    case RegionType::REGION_TYPE_CHORD:
      return type == TrackType::TRACK_TYPE_CHORD;
    case RegionType::REGION_TYPE_AUTOMATION:
      return true;
    }

  g_return_val_if_reached (false);
}

static inline bool
track_type_is_foldable (TrackType type)
{
  return type == TrackType::TRACK_TYPE_FOLDER
         || type == TrackType::TRACK_TYPE_MIDI_GROUP
         || type == TrackType::TRACK_TYPE_AUDIO_GROUP;
}

static inline bool
track_type_is_copyable (TrackType type)
{
  return type != TrackType::TRACK_TYPE_MASTER && type != TrackType::TRACK_TYPE_TEMPO
         && type != TrackType::TRACK_TYPE_CHORD
         && type != TrackType::TRACK_TYPE_MODULATOR
         && type != TrackType::TRACK_TYPE_MARKER;
}

/**
 * Sets magic on objects recursively.
 */
NONNULL void
track_set_magic (Track * self);

#define track_get_name_hash(self) g_str_hash (self->name)

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
NONNULL void
track_set_muted (
  Track * track,
  bool    mute,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events);

/**
 * Sets track folded and optionally adds the action
 * to the undo stack.
 */
NONNULL void
track_set_folded (
  Track * self,
  bool    folded,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events);

NONNULL TrackType
track_get_type_from_plugin_descriptor (PluginDescriptor * descr);

/**
 * Returns whether the track type is deletable
 * by the user.
 */
NONNULL bool
track_type_is_deletable (TrackType type);

NONNULL Tracklist *
track_get_tracklist (Track * self);

/**
 * Returns whether the track should be visible.
 *
 * Takes into account Track.visible and whether
 * any of the track's foldable parents are folded.
 */
NONNULL bool
track_get_should_be_visible (const Track * self);

/**
 * Returns the full visible height (main height + height of
 * all visible automation tracks + height of all visible
 * lanes).
 *
 * @memberof Track
 */
NONNULL double
track_get_full_visible_height (Track * const self);

bool
track_multiply_heights (
  Track * self,
  double  multiplier,
  bool    visible_only,
  bool    check_only);

/**
 * Returns if the track is soloed.
 */
HOT NONNULL bool
track_get_soloed (Track * self);

/**
 * Returns whether the track is not soloed on its
 * own but its direct out (or its direct out's direct
 * out, etc.) is soloed.
 */
NONNULL bool
track_get_implied_soloed (Track * self);

/**
 * Returns if the track is muted.
 */
NONNULL bool
track_get_muted (Track * self);

/**
 * Returns if the track is listened.
 */
NONNULL bool
track_get_listened (Track * self);

/**
 * Returns whether monitor audio is on.
 */
NONNULL bool
track_get_monitor_audio (Track * self);

/**
 * Sets whether monitor audio is on.
 */
NONNULL void
track_set_monitor_audio (
  Track * self,
  bool    monitor,
  bool    auto_select,
  bool    fire_events);

/**
 * Sets track soloed, updates UI and optionally
 * adds the action to the undo stack.
 *
 * @param auto_select Makes this track the only
 *   selection in the tracklist. Useful when
 *   listening to a single track.
 * @param trigger_undo Create and perform an
 *   undoable action.
 * @param fire_events Fire UI events.
 */
NONNULL void
track_set_listened (
  Track * self,
  bool    listen,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events);

HOT NONNULL bool
track_get_recording (const Track * const track);

/**
 * Sets recording and connects/disconnects the
 * JACK ports.
 */
NONNULL void
track_set_recording (Track * track, bool recording, bool fire_events);

/**
 * Sets track soloed, updates UI and optionally
 * adds the action to the undo stack.
 *
 * @param auto_select Makes this track the only
 *   selection in the tracklist. Useful when soloing
 *   a single track.
 * @param trigger_undo Create and perform an
 *   undoable action.
 * @param fire_events Fire UI events.
 */
NONNULL void
track_set_soloed (
  Track * self,
  bool    solo,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events);

/**
 * Returns whether the track has any soloed lanes.
 */
NONNULL bool
track_has_soloed_lanes (const Track * const self);

/**
 * Returns if Track is in TracklistSelections.
 */
NONNULL int
track_is_selected (Track * self);

/**
 * Returns whether the track is pinned.
 */
#define track_is_pinned(x) (x->pos < TRACKLIST->pinned_tracks_cutoff)

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
#define track_add_region( \
  self, region, at, lane_pos, gen_name, fire_events, error) \
  track_insert_region ( \
    self, region, at, lane_pos, -1, gen_name, fire_events, error)

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
 *
 * @return Whether successful.
 */
bool
track_insert_region (
  Track *           track,
  ZRegion *         region,
  AutomationTrack * at,
  int               lane_pos,
  int               idx,
  int               gen_name,
  int               fire_events,
  GError **         error);

/**
 * Writes the track to the given MIDI file.
 *
 * @param use_track_pos Whether to use the track position in
 *   the MIDI data. The track will be set to 1 if false.
 * @param events Track events, if not using lanes as tracks or
 *   using track position.
 * @param start Events before this position will be skipped.
 * @param end Events after this position will be skipped.
 */
NONNULL_ARGS (1, 2)
void track_write_to_midi_file (
  const Track *    self,
  MIDI_FILE *      mf,
  MidiEvents *     events,
  const Position * start,
  const Position * end,
  bool             lanes_as_tracks,
  bool             use_track_pos);

/**
 * Appends the Track to the selections.
 *
 * @param exclusive Select only this track.
 * @param fire_events Fire events to update the
 *   UI.
 */
NONNULL void
track_select (Track * self, bool select, bool exclusive, bool fire_events);

/**
 * Unselects all arranger objects in the track.
 */
NONNULL void
track_unselect_all (Track * self);

NONNULL bool
track_contains_uninstantiated_plugin (const Track * const self);

/**
 * Removes all objects recursively from the track.
 */
NONNULL void
track_clear (Track * self);

/**
 * Only removes the region from the track.
 *
 * @pararm free Also free the Region.
 */
NONNULL void
track_remove_region (Track * self, ZRegion * region, bool fire_events, bool free);

/**
 * Wrapper for audio and MIDI/instrument tracks to fill in
 * MidiEvents or StereoPorts from the timeline data.
 *
 * @note The engine splits the cycle so transport loop
 *   related logic is not needed.
 *
 * @param stereo_ports StereoPorts to fill.
 * @param midi_events MidiEvents to fill (from Piano Roll Port
 *   for example).
 */
void
track_fill_events (
  const Track *                       self,
  const EngineProcessTimeInfo * const time_nfo,
  MidiEvents *                        midi_events,
  StereoPorts *                       stereo_ports);

/**
 * Verifies the identifiers on a live Track
 * (in the project, not a clone).
 *
 * @return True if pass.
 */
bool
track_validate (Track * self);

/**
 * Adds the track's folder parents to the given
 * array.
 *
 * @param prepend Whether to prepend instead of
 *   append.
 */
void
track_add_folder_parents (const Track * self, GPtrArray * parents, bool prepend);

/**
 * Returns the closest foldable parent or NULL.
 */
Track *
track_get_direct_folder_parent (Track * track);

/**
 * Remove the track from all folders.
 *
 * Used when deleting tracks.
 */
void
track_remove_from_folder_parents (Track * self);

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
track_get_last_region (Track * track);

/**
 * Set track lanes visible and fire events.
 */
void
track_set_lanes_visible (Track * track, const int visible);

/**
 * Set automation visible and fire events.
 */
void
track_set_automation_visible (Track * track, const bool visible);

/**
 * Returns if the given TrackType can host the
 * given RegionType.
 */
int
track_type_can_host_region_type (const TrackType tt, const RegionType rt);

static inline bool
track_type_has_mono_compat_switch (const TrackType tt)
{
  return tt == TrackType::TRACK_TYPE_AUDIO_GROUP
         || tt == TrackType::TRACK_TYPE_MASTER;
}

#define track_type_is_audio_group track_type_has_mono_compat_switch

static inline bool
track_type_is_fx (const TrackType type)
{
  return type == TrackType::TRACK_TYPE_AUDIO_BUS
         || type == TrackType::TRACK_TYPE_MIDI_BUS;
}

/**
 * Returns if the Track can record.
 */
static inline int
track_type_can_record (const TrackType type)
{
  return type == TrackType::TRACK_TYPE_AUDIO || type == TrackType::TRACK_TYPE_MIDI
         || type == TrackType::TRACK_TYPE_CHORD
         || type == TrackType::TRACK_TYPE_INSTRUMENT;
}

/**
 * Generates automatables for the track.
 *
 * Should be called as soon as the track is
 * created.
 */
void
track_generate_automation_tracks (Track * track);

/**
 * Wrapper.
 */
void
track_setup (Track * track);

/**
 * Returns the automation tracklist if the track type has one,
 * or NULL if it doesn't (like chord tracks).
 */
static inline AutomationTracklist *
track_get_automation_tracklist (Track * const track)
{
  g_return_val_if_fail (IS_TRACK (track), NULL);

  switch (track->type)
    {
    case TrackType::TRACK_TYPE_MARKER:
    case TrackType::TRACK_TYPE_FOLDER:
      break;
    case TrackType::TRACK_TYPE_CHORD:
    case TrackType::TRACK_TYPE_AUDIO_BUS:
    case TrackType::TRACK_TYPE_AUDIO_GROUP:
    case TrackType::TRACK_TYPE_MIDI_BUS:
    case TrackType::TRACK_TYPE_MIDI_GROUP:
    case TrackType::TRACK_TYPE_INSTRUMENT:
    case TrackType::TRACK_TYPE_AUDIO:
    case TrackType::TRACK_TYPE_MASTER:
    case TrackType::TRACK_TYPE_MIDI:
    case TrackType::TRACK_TYPE_TEMPO:
    case TrackType::TRACK_TYPE_MODULATOR:
      return &track->automation_tracklist;
    default:
      g_warn_if_reached ();
      break;
    }

  return NULL;
}

/**
 * Returns the channel of the track, if the track
 * type has a channel, or NULL if it doesn't.
 */
NONNULL static inline Channel *
track_get_channel (const Track * const track)
{
  switch (track->type)
    {
    case TrackType::TRACK_TYPE_MASTER:
    case TrackType::TRACK_TYPE_INSTRUMENT:
    case TrackType::TRACK_TYPE_AUDIO:
    case TrackType::TRACK_TYPE_AUDIO_BUS:
    case TrackType::TRACK_TYPE_AUDIO_GROUP:
    case TrackType::TRACK_TYPE_MIDI_BUS:
    case TrackType::TRACK_TYPE_MIDI_GROUP:
    case TrackType::TRACK_TYPE_MIDI:
    case TrackType::TRACK_TYPE_CHORD:
      return track->channel;
    default:
      return NULL;
    }
}

/**
 * Updates the track's children.
 *
 * Used when changing track positions.
 */
void
track_update_children (Track * self);

/**
 * Returns if the Track should have a piano roll.
 */
CONST
static inline bool
track_type_has_piano_roll (const TrackType type)
{
  return type == TrackType::TRACK_TYPE_MIDI
         || type == TrackType::TRACK_TYPE_INSTRUMENT;
}

/**
 * Returns if the Track should have an inputs
 * selector.
 */
static inline int
track_has_inputs (const Track * track)
{
  return track->type == TrackType::TRACK_TYPE_MIDI
         || track->type == TrackType::TRACK_TYPE_INSTRUMENT
         || track->type == TrackType::TRACK_TYPE_AUDIO;
}

Track *
track_find_by_name (const char * name);

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
 * Internally called by
 * track_set_name_with_action().
 */
NONNULL bool
track_set_name_with_action_full (Track * track, const char * name);

/**
 * Setter to be used by the UI to create an
 * undoable action.
 */
void
track_set_name_with_action (Track * track, const char * name);

/**
 * Setter for the track name.
 *
 * If a track with that name already exists, it
 * adds a number at the end.
 *
 * Must only be called from the GTK thread.
 */
void
track_set_name (Track * self, const char * name, bool pub_events);

/**
 * Returns a unique name for a new track based on
 * the given name.
 */
char *
track_get_unique_name (Track * track_to_skip, const char * name);

/**
 * Returns the Track from the Project matching
 * \p name.
 *
 * @param name Name to search for.
 */
Track *
track_get_from_name (const char * name);

const char *
track_stringize_type (TrackType type);

/**
 * Returns if regions in tracks from type1 can
 * be moved to type2.
 */
static inline int
track_type_is_compatible_for_moving (const TrackType type1, const TrackType type2)
{
  return type1 == type2
         || (type1 == TrackType::TRACK_TYPE_MIDI && type2 == TrackType::TRACK_TYPE_INSTRUMENT)
         || (type1 == TrackType::TRACK_TYPE_INSTRUMENT && type2 == TrackType::TRACK_TYPE_MIDI);
}

/**
 * Updates the frames/ticks of each position in
 * each child of the track recursively.
 *
 * @param from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 */
void
track_update_positions (Track * self, bool from_ticks, bool bpm_change);

/**
 * Returns the Fader (if applicable).
 *
 * @param post_fader True to get post fader,
 *   false to get pre fader.
 */
Fader *
track_get_fader (Track * track, bool post_fader);

/**
 * Returns the FaderType corresponding to the given
 * Track.
 */
FaderType
track_get_fader_type (const Track * track);

/**
 * Returns the prefader type
 * corresponding to the given Track.
 */
FaderType
track_type_get_prefader_type (TrackType type);

/**
 * Creates missing TrackLane's until pos.
 *
 * @return Whether a new lane was created.
 */
bool
track_create_missing_lanes (Track * self, const int pos);

/**
 * Removes the empty last lanes of the Track
 * (except the last one).
 */
void
track_remove_empty_last_lanes (Track * self);

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

/**
 * Fills in the given array (if non-NULL) with all
 * plugins in the track and returns the number of
 * plugins.
 */
int
track_get_plugins (const Track * const self, GPtrArray * arr);

void
track_activate_all_plugins (Track * track, bool activate);

/**
 * Comment setter.
 *
 * @note This creates an undoable action.
 */
void
track_comment_setter (void * track, const char * comment);

/**
 * @param undoable Create an undable action.
 */
void
track_set_comment (Track * self, const char * comment, bool undoable);

/**
 * Comment getter.
 */
const char *
track_get_comment (void * track);

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
 * Returns the plugin at the given slot, if any.
 *
 * @param slot The slot (ignored if instrument is
 *   selected.
 */
Plugin *
track_get_plugin_at_slot (Track * track, ZPluginSlotType slot_type, int slot);

/**
 * Marks the track for bouncing.
 *
 * @param mark_children Whether to mark all children tracks
 *   as well. Used when exporting stems on the specific track
 *   stem only.
 *   IMPORTANT: Track.bounce_to_master must be set beforehand
 *   if this is true.
 * @param mark_parents Whether to mark all parent tracks as
 *   well.
 */
void
track_mark_for_bounce (
  Track * self,
  bool    bounce,
  bool    mark_regions,
  bool    mark_children,
  bool    mark_parents);

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 */
void
track_append_ports (Track * self, GPtrArray * ports, bool include_plugins);

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
 *
 * @return Whether successful.
 */
bool
track_freeze (Track * self, bool freeze, GError ** error);

/**
 * Wrapper over channel_add_plugin() and
 * modulator_track_insert_modulator().
 *
 * @param instantiate_plugin Whether to attempt to
 *   instantiate the plugin.
 */
void
track_insert_plugin (
  Track *         self,
  Plugin *        pl,
  ZPluginSlotType slot_type,
  int             slot,
  bool            instantiate_plugin,
  bool            replacing_plugin,
  bool            moving_plugin,
  bool            confirm,
  bool            gen_automatables,
  bool            recalc_graph,
  bool            fire_events);

/**
 * Wrapper over channel_remove_plugin() and
 * modulator_track_remove_modulator().
 */
void
track_remove_plugin (
  Track *         self,
  ZPluginSlotType slot_type,
  int             slot,
  bool            replacing_plugin,
  bool            moving_plugin,
  bool            deleting_plugin,
  bool            deleting_track,
  bool            recalc_graph);

/**
 * Disconnects the track from the processing chain.
 *
 * This should be called immediately when the track is getting deleted, and
 * track_free should be designed to be called later after an arbitrary delay.
 *
 * @param remove_pl Remove the Plugin from the
 *   Channel. Useful when deleting the channel.
 * @param recalc_graph Recalculate mixer graph.
 */
void
track_disconnect (Track * self, bool remove_pl, bool recalc_graph);

NONNULL bool
track_is_enabled (Track * self);

NONNULL void
track_set_enabled (
  Track * self,
  bool    enabled,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events);

static inline const char *
track_type_to_string (TrackType type)
{
  return track_type_strings[static_cast<int> (type)];
}

TrackType
track_type_get_from_string (const char * str);

void
track_get_total_bars (Track * self, int * total_bars);

/**
 * Set various caches (snapshots, track name hash, plugin
 * input/output ports, etc).
 */
void
track_set_caches (Track * self, CacheTypes types);

/**
 * Called when track(s) are actually imported into the
 * project.
 */
typedef void (*TracksReadyCallback) (const FileImportInfo *, const GError *);

/**
 * @param disable_track_idx Track index to disable, or -1.
 * @param ready_cb Callback to be called when the tracks are
 *   ready (added to the project).
 */
bool
track_create_with_action (
  TrackType             type,
  const PluginSetting * pl_setting,
  const SupportedFile * file_descr,
  const Position *      pos,
  int                   index,
  int                   num_tracks,
  int                   disable_track_idx,
  TracksReadyCallback   ready_cb,
  GError **             error);

Track *
track_create_empty_at_idx_with_action (TrackType type, int index, GError ** error);

Track *
track_create_for_plugin_at_idx_w_action (
  TrackType             type,
  const PluginSetting * pl_setting,
  int                   index,
  GError **             error);

/**
 * Creates a new empty track at the end of the
 * tracklist.
 */
#define track_create_empty_with_action(type, error) \
  track_create_empty_at_idx_with_action (type, TRACKLIST->num_tracks, error)

GMenu *
track_generate_edit_context_menu (Track * track, int num_selected);

/**
 * Generates a menu to be used for channel-related items, eg,
 * fader buttons, direct out, etc.
 */
GMenu *
track_generate_channel_context_menu (Track * track);

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track);

/**
 * @}
 */

#endif // __AUDIO_TRACK_H__
