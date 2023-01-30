// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "actions/tracklist_selections.h"
#include "actions/undo_manager.h"
#include "audio/audio_bus_track.h"
#include "audio/audio_group_track.h"
#include "audio/audio_region.h"
#include "audio/audio_track.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/control_port.h"
#include "audio/exporter.h"
#include "audio/foldable_track.h"
#include "audio/group_target_track.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_bus_track.h"
#include "audio/midi_event.h"
#include "audio/midi_group_track.h"
#include "audio/midi_track.h"
#include "audio/modulator_track.h"
#include "audio/router.h"
#include "audio/stretcher.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "midilib/src/midifile.h"
#include "midilib/src/midiinfo.h"

void
track_init_loaded (
  Track *               self,
  Tracklist *           tracklist,
  TracklistSelections * ts)
{
  self->magic = TRACK_MAGIC;
  self->tracklist = tracklist;
  self->ts = ts;

  if (TRACK_CAN_BE_GROUP_TARGET (self))
    {
      group_target_track_init_loaded (self);
    }
  else if (self->type == TRACK_TYPE_MODULATOR)
    {
      for (int i = 0; i < self->num_modulators; i++)
        {
          plugin_init_loaded (self->modulators[i], self, NULL);
        }
    }

  TrackLane * lane;
  for (int j = 0; j < self->num_lanes; j++)
    {
      lane = self->lanes[j];
      track_lane_init_loaded (lane, self);
    }
  self->lanes_size = (size_t) self->num_lanes;
  ScaleObject * scale;
  for (int i = 0; i < self->num_scales; i++)
    {
      scale = self->scales[i];
      arranger_object_init_loaded ((ArrangerObject *) scale);
    }
  Marker * marker;
  for (int i = 0; i < self->num_markers; i++)
    {
      marker = self->markers[i];
      arranger_object_init_loaded ((ArrangerObject *) marker);
    }
  ZRegion * region;
  for (int i = 0; i < self->num_chord_regions; i++)
    {
      region = self->chord_regions[i];
      region->id.track_name_hash = track_get_name_hash (self);
      arranger_object_init_loaded ((ArrangerObject *) region);
    }

  /* init loaded track processor */
  if (self->processor)
    {
      self->processor->track = self;
      track_processor_init_loaded (self->processor, self);
    }

  /* init loaded channel */
  if (self->channel)
    {
      self->channel->track = self;
      channel_init_loaded (self->channel, self);
    }

  /* set track to automation tracklist */
  AutomationTracklist * atl =
    track_get_automation_tracklist (self);
  if (atl)
    {
      automation_tracklist_init_loaded (atl, self);
    }

  if (self->type == TRACK_TYPE_AUDIO)
    {
      self->rt_stretcher = stretcher_new_rubberband (
        AUDIO_ENGINE->sample_rate, 2, 1.0, 1.0, true);
    }

  for (int i = 0; i < self->num_modulator_macros; i++)
    {
      ModulatorMacroProcessor * mmp =
        self->modulator_macros[i];
      modulator_macro_processor_init_loaded (mmp, self);
    }

  /** set magic to all track ports */
  GPtrArray * ports = g_ptr_array_new ();
  track_append_ports (self, ports, true);
  unsigned int name_hash = track_get_name_hash (self);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      port->magic = PORT_MAGIC;
      port->track = self;
      if (track_is_in_active_project (self))
        {
          g_return_if_fail (
            port->id.track_name_hash == name_hash);

          /* set automation tracks on ports */
          if (port->id.flags & PORT_FLAG_AUTOMATABLE)
            {
              AutomationTrack * at =
                automation_track_find_from_port (
                  port, self, true);
              g_return_if_fail (at);
              port->at = at;
            }
        }
    }
  object_free_w_func_and_null (g_ptr_array_unref, ports);
}

/**
 * Adds a new TrackLane to the Track.
 */
NONNULL static void
track_add_lane (Track * self, int fire_events)
{
  g_return_if_fail (IS_TRACK (self));

  array_double_size_if_full (
    self->lanes, self->num_lanes, self->lanes_size,
    TrackLane *);
  TrackLane * lane = track_lane_new (self, self->num_lanes);
  g_return_if_fail (lane);
  self->lanes[self->num_lanes++] = lane;

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_TRACK_LANE_ADDED, self->lanes[self->num_lanes - 1]);
    }
}

/**
 * Inits the Track, optionally adding a single
 * lane.
 *
 * @param add_lane Add a lane. This should be used
 *   for new Tracks. When cloning, the lanes should
 *   be cloned so this should be 0.
 */
void
track_init (Track * self, const int add_lane)
{
  self->schema_version = TRACK_SCHEMA_VERSION;
  self->visible = true;
  self->main_height = TRACK_DEF_HEIGHT;
  self->midi_ch = 1;
  self->magic = TRACK_MAGIC;
  self->enabled = true;
  self->comment = g_strdup ("");
  self->size = 1;
  track_add_lane (self, 0);
}

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
  const int    with_lane)
{
  Track * self = object_new (Track);

  g_debug ("creating track '%s'", label);

  self->pos = pos;
  self->type = type;
  track_init (self, with_lane);

  self->name = g_strdup (label);

  switch (type)
    {
    case TRACK_TYPE_INSTRUMENT:
      self->in_signal_type = TYPE_EVENT;
      self->out_signal_type = TYPE_AUDIO;
      instrument_track_init (self);
      break;
    case TRACK_TYPE_AUDIO:
      self->in_signal_type = TYPE_AUDIO;
      self->out_signal_type = TYPE_AUDIO;
      audio_track_init (self);
      break;
    case TRACK_TYPE_MASTER:
      self->in_signal_type = TYPE_AUDIO;
      self->out_signal_type = TYPE_AUDIO;
      master_track_init (self);
      break;
    case TRACK_TYPE_AUDIO_BUS:
      self->in_signal_type = TYPE_AUDIO;
      self->out_signal_type = TYPE_AUDIO;
      audio_bus_track_init (self);
      break;
    case TRACK_TYPE_MIDI_BUS:
      self->in_signal_type = TYPE_EVENT;
      self->out_signal_type = TYPE_EVENT;
      midi_bus_track_init (self);
      break;
    case TRACK_TYPE_AUDIO_GROUP:
      self->in_signal_type = TYPE_AUDIO;
      self->out_signal_type = TYPE_AUDIO;
      audio_group_track_init (self);
      break;
    case TRACK_TYPE_MIDI_GROUP:
      self->in_signal_type = TYPE_EVENT;
      self->out_signal_type = TYPE_EVENT;
      midi_group_track_init (self);
      break;
    case TRACK_TYPE_MIDI:
      self->in_signal_type = TYPE_EVENT;
      self->out_signal_type = TYPE_EVENT;
      midi_track_init (self);
      break;
    case TRACK_TYPE_CHORD:
      self->in_signal_type = TYPE_EVENT;
      self->out_signal_type = TYPE_EVENT;
      chord_track_init (self);
      break;
    case TRACK_TYPE_MARKER:
      self->in_signal_type = TYPE_UNKNOWN;
      self->out_signal_type = TYPE_UNKNOWN;
      marker_track_init (self);
      break;
    case TRACK_TYPE_TEMPO:
      self->in_signal_type = TYPE_UNKNOWN;
      self->out_signal_type = TYPE_UNKNOWN;
      tempo_track_init (self);
      break;
    case TRACK_TYPE_MODULATOR:
      self->in_signal_type = TYPE_UNKNOWN;
      self->out_signal_type = TYPE_UNKNOWN;
      modulator_track_init (self);
      break;
    case TRACK_TYPE_FOLDER:
      self->in_signal_type = TYPE_UNKNOWN;
      self->out_signal_type = TYPE_UNKNOWN;
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  if (track_type_is_foldable (type))
    {
      foldable_track_init (self);
    }

  if (TRACK_CAN_BE_GROUP_TARGET (self))
    {
      group_target_track_init (self);
    }

  if (track_type_can_record (type))
    {
      self->recording = port_new_with_type_and_owner (
        TYPE_CONTROL, FLOW_INPUT, _ ("Track record"),
        PORT_OWNER_TYPE_TRACK, self);
      self->recording->id.sym = g_strdup ("track_record");
      control_port_set_toggled (
        self->recording, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
      self->recording->id.flags2 |= PORT_FLAG2_TRACK_RECORDING;
      self->recording->id.flags |= PORT_FLAG_TOGGLE;
    }

  self->processor = track_processor_new (self);

  automation_tracklist_init (
    &self->automation_tracklist, self);

  if (track_type_has_channel (self->type))
    {
      self->channel = channel_new (self);
    }

  track_generate_automation_tracks (self);

  return self;
}

/**
 * Returns if the given TrackType is a type of
 * Track that has a Channel.
 */
bool
track_type_has_channel (TrackType type)
{
  switch (type)
    {
    case TRACK_TYPE_MARKER:
    case TRACK_TYPE_TEMPO:
    case TRACK_TYPE_MODULATOR:
    case TRACK_TYPE_FOLDER:
      return false;
    default:
      break;
    }

  return true;
}

/**
 * Returns whether the track type is deletable
 * by the user.
 */
bool
track_type_is_deletable (TrackType type)
{
  return type != TRACK_TYPE_MASTER && type != TRACK_TYPE_TEMPO
         && type != TRACK_TYPE_CHORD && type != TRACK_TYPE_MODULATOR
         && type != TRACK_TYPE_MARKER;
}

/**
 * Clones the track and returns the clone.
 *
 * @param error To be filled if an error occurred.
 */
Track *
track_clone (Track * track, GError ** error)
{
  g_return_val_if_fail (!error || !*error, NULL);

  /* set the track on the given track's channel
   * if not set */
  if (
    !track_is_in_active_project (track) && track->channel
    && !track->channel->track)
    track->channel->track = track;

  Track * new_track = track_new (
    track->type, track->pos, track->name, F_WITHOUT_LANE);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (new_track), NULL);

  new_track->icon_name = g_strdup (track->icon_name);
  new_track->comment = g_strdup (track->comment);

#define COPY_MEMBER(a) new_track->a = track->a

  COPY_MEMBER (type);
  COPY_MEMBER (automation_visible);
  COPY_MEMBER (lanes_visible);
  COPY_MEMBER (visible);
  COPY_MEMBER (main_height);
  COPY_MEMBER (enabled);
  COPY_MEMBER (color);
  COPY_MEMBER (pos);
  COPY_MEMBER (midi_ch);
  COPY_MEMBER (size);
  COPY_MEMBER (folded);
  COPY_MEMBER (record_set_automatically);
  COPY_MEMBER (drum_mode);

#undef COPY_MEMBER

  if (track->recording)
    {
      control_port_set_toggled (
        new_track->recording,
        control_port_is_toggled (track->recording),
        F_NO_PUBLISH_EVENTS);
    }

  if (track->type == TRACK_TYPE_MODULATOR)
    {
      for (int i = 0; i < track->num_modulators; i++)
        {
          GError * err = NULL;
          Plugin * pl = track->modulators[i];
          Plugin * clone_pl = plugin_clone (pl, &err);
          if (!clone_pl)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, "%s",
                _ ("Failed to clone modulator "
                   "plugin"));
              object_free_w_func_and_null (
                track_free, new_track);
              return NULL;
            }
          modulator_track_insert_modulator (
            new_track, i, clone_pl, F_REPLACING, F_NO_CONFIRM,
            F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);
        }
      z_return_val_if_fail_cmp (
        new_track->num_modulators, ==, track->num_modulators,
        NULL);
    }

  if (track->channel)
    {
      object_free_w_func_and_null (
        channel_free, new_track->channel);

      /* set the given channel's track if not
       * set */
      GError * err = NULL;
      new_track->channel =
        channel_clone (track->channel, new_track, &err);
      if (new_track->channel == NULL)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s", _ ("Failed to clone channel"));
          object_free_w_func_and_null (track_free, new_track);
          return NULL;
        }
      new_track->channel->track = new_track;
    }

  /* --- copy objects --- */

  new_track->num_lanes = track->num_lanes;
  new_track->lanes_size = (size_t) new_track->num_lanes;
  new_track->lanes = g_realloc_n (
    new_track->lanes, new_track->lanes_size,
    sizeof (TrackLane *));
  for (int j = 0; j < track->num_lanes; j++)
    {
      TrackLane * lane = track->lanes[j];
      TrackLane * new_lane =
        track_lane_clone (lane, new_track);
      new_track->lanes[j] = new_lane;
    }

  new_track->num_scales = track->num_scales;
  new_track->scales = g_realloc_n (
    new_track->scales, (size_t) track->num_scales,
    sizeof (ScaleObject *));
  for (int j = 0; j < track->num_scales; j++)
    {
      ScaleObject * s = track->scales[j];
      new_track->scales[j] = (ScaleObject *)
        arranger_object_clone ((ArrangerObject *) s);
    }

  new_track->num_markers = track->num_markers;
  new_track->markers = g_realloc_n (
    new_track->markers, (size_t) track->num_markers,
    sizeof (Marker *));
  for (int j = 0; j < track->num_markers; j++)
    {
      Marker * m = track->markers[j];
      new_track->markers[j] = (Marker *)
        arranger_object_clone ((ArrangerObject *) m);
    }

  new_track->num_chord_regions = track->num_chord_regions;
  new_track->chord_regions = g_realloc_n (
    new_track->chord_regions,
    (size_t) track->num_chord_regions, sizeof (ZRegion *));
  for (int j = 0; j < track->num_chord_regions; j++)
    {
      ZRegion * r = track->chord_regions[j];
      new_track->chord_regions[j] = (ZRegion *)
        arranger_object_clone ((ArrangerObject *) r);
    }

  new_track->automation_tracklist.track = new_track;
  automation_tracklist_clone (
    &track->automation_tracklist,
    &new_track->automation_tracklist);

  /* --- end copy objects --- */

  if (TRACK_CAN_BE_GROUP_TARGET (track))
    {
      for (int i = 0; i < track->num_children; i++)
        {
          group_target_track_add_child (
            new_track, track->children[i], false,
            F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
        }
    }

  /* copy processor values */
  if (track->processor)
    {
      track_processor_copy_values (
        new_track->processor, track->processor);
    }

#define COPY_PORT_VALUES(x) \
  if (track->x) \
  port_copy_values (new_track->x, track->x)

  /* copy own values */
  COPY_PORT_VALUES (recording);
  COPY_PORT_VALUES (bpm_port);
  COPY_PORT_VALUES (beats_per_bar_port);
  COPY_PORT_VALUES (beat_unit_port);

#undef COPY_PORT_VALUES

  /* check that source track is not affected
   * during unit tests */
  if (ZRYTHM_TESTING && track_is_in_active_project (track))
    {
      track_validate (track);
    }

  return new_track;
}

/**
 * Sets magic on objects recursively.
 */
void
track_set_magic (Track * self)
{
  self->magic = TRACK_MAGIC;

  if (track_type_has_channel (self->type))
    {
      Channel * ch = self->channel;
      g_return_if_fail (ch);

      channel_set_magic (ch);
    }
}

Tracklist *
track_get_tracklist (Track * self)
{
  if (track_is_auditioner (self))
    {
      return SAMPLE_PROCESSOR->tracklist;
    }
  else
    {
      return TRACKLIST;
    }
}

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
  bool    select,
  bool    exclusive,
  bool    fire_events)
{
  g_return_if_fail (IS_TRACK (self));

  if (select)
    {
      if (exclusive)
        {
          tracklist_selections_select_single (
            TRACKLIST_SELECTIONS, self, fire_events);
        }
      else
        {
          tracklist_selections_add_track (
            TRACKLIST_SELECTIONS, self, fire_events);
        }
    }
  else
    {
      tracklist_selections_remove_track (
        TRACKLIST_SELECTIONS, self, fire_events);
    }

  if (fire_events)
    {
      EVENTS_PUSH (ET_TRACK_CHANGED, self);
    }
}

/**
 * Returns if the track is soloed.
 */
bool
track_get_soloed (Track * self)
{
  if (G_UNLIKELY (self->type == TRACK_TYPE_FOLDER))
    {
      return foldable_track_is_status (
        self, FOLDABLE_TRACK_MIXER_STATUS_SOLOED);
    }

  g_return_val_if_fail (self->channel, false);
  return fader_get_soloed (self->channel->fader);
}

/**
 * Returns whether the track is not soloed on its
 * own but its direct out (or its direct out's direct
 * out, etc.) is soloed.
 */
bool
track_get_implied_soloed (Track * self)
{
  if (self->type == TRACK_TYPE_FOLDER)
    {
      return foldable_track_is_status (
        self, FOLDABLE_TRACK_MIXER_STATUS_IMPLIED_SOLOED);
    }

  g_return_val_if_fail (self->channel, false);
  return fader_get_implied_soloed (self->channel->fader);
}

/**
 * Returns if the track is muted.
 */
bool
track_get_muted (Track * self)
{
  if (self->type == TRACK_TYPE_FOLDER)
    {
      return foldable_track_is_status (
        self, FOLDABLE_TRACK_MIXER_STATUS_MUTED);
    }

  g_return_val_if_fail (self->channel, false);
  return fader_get_muted (self->channel->fader);
}

/**
 * Returns if the track is listened.
 */
bool
track_get_listened (Track * self)
{
  if (self->type == TRACK_TYPE_FOLDER)
    {
      return foldable_track_is_status (
        self, FOLDABLE_TRACK_MIXER_STATUS_LISTENED);
    }

  g_return_val_if_fail (self->channel, false);
  return fader_get_listened (self->channel->fader);
}

/**
 * Returns whether monitor audio is on.
 */
bool
track_get_monitor_audio (Track * self)
{
  g_return_val_if_fail (
    IS_TRACK (self) && self->processor
      && IS_PORT_AND_NONNULL (self->processor->monitor_audio),
    false);

  return control_port_is_toggled (
    self->processor->monitor_audio);
}

/**
 * Sets whether monitor audio is on.
 */
void
track_set_monitor_audio (
  Track * self,
  bool    monitor,
  bool    auto_select,
  bool    fire_events)
{
  g_return_if_fail (
    IS_TRACK (self) && self->processor
    && IS_PORT_AND_NONNULL (self->processor->monitor_audio));

  if (auto_select)
    {
      track_select (self, F_SELECT, F_EXCLUSIVE, fire_events);
    }

  control_port_set_toggled (
    self->processor->monitor_audio, monitor, fire_events);
}

TrackType
track_get_type_from_plugin_descriptor (
  PluginDescriptor * descr)
{
  if (plugin_descriptor_is_instrument (descr))
    return TRACK_TYPE_INSTRUMENT;
  else if (plugin_descriptor_is_midi_modifier (descr))
    return TRACK_TYPE_MIDI;
  else
    return TRACK_TYPE_AUDIO_BUS;
}

bool
track_get_recording (const Track * const track)
{
  g_return_val_if_fail (
    IS_TRACK (track) && track_type_can_record (track->type)
      && IS_PORT_AND_NONNULL (track->recording),
    false);

  return control_port_is_toggled (track->recording);
}

/**
 * Sets recording and connects/disconnects the
 * JACK ports.
 */
void
track_set_recording (
  Track * track,
  bool    recording,
  bool    fire_events)
{
  g_return_if_fail (IS_TRACK (track));

  g_debug (
    "%s: setting recording %d (fire events: %d)", track->name,
    recording, fire_events);

  Channel * channel = track_get_channel (track);

  if (!channel)
    {
      g_critical (
        "Recording not implemented yet for this "
        "track.");
      return;
    }

  control_port_set_toggled (
    track->recording, recording, F_NO_PUBLISH_EVENTS);

  if (recording)
    {
      g_message ("enabled recording on %s", track->name);
    }
  else
    {
      g_message ("disabled recording on %s", track->name);

      /* send all notes off if can record MIDI */
      track->processor->pending_midi_panic = true;
    }

  if (fire_events)
    {
      EVENTS_PUSH (ET_TRACK_STATE_CHANGED, track);
    }
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
track_set_muted (
  Track * self,
  bool    mute,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events)
{
  g_message ("Setting track %s muted (%d)", self->name, mute);

  if (auto_select)
    {
      track_select (self, F_SELECT, F_EXCLUSIVE, fire_events);
    }

  if (trigger_undo)
    {
      /* this is only supported if the fader track
       * is the only track selected */
      g_return_if_fail (
        TRACKLIST_SELECTIONS->num_tracks == 1
        && TRACKLIST_SELECTIONS->tracks[0] == self);

      GError * err = NULL;
      bool ret = tracklist_selections_action_perform_edit_mute (
        TRACKLIST_SELECTIONS, mute, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track muted"));
          return;
        }
    }
  else
    {
      fader_set_muted (
        self->channel->fader, mute, fire_events);
    }
}

/**
 * Sets track folded and optionally adds the action
 * to the undo stack.
 */
void
track_set_folded (
  Track * self,
  bool    folded,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events)
{
  g_return_if_fail (track_type_is_foldable (self->type));

  g_message (
    "Setting track %s folded (%d)", self->name, folded);
  if (auto_select)
    {
      track_select (self, F_SELECT, F_EXCLUSIVE, fire_events);
    }

  if (trigger_undo)
    {
      g_return_if_fail (
        TRACKLIST_SELECTIONS->num_tracks == 1
        && TRACKLIST_SELECTIONS->tracks[0] == self);

      GError * err = NULL;
      bool ret = tracklist_selections_action_perform_edit_fold (
        TRACKLIST_SELECTIONS, folded, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track folded"));
          return;
        }
    }
  else
    {
      self->folded = folded;

      if (fire_events)
        {
          EVENTS_PUSH (ET_TRACK_FOLD_CHANGED, self);
        }
    }
}

/**
 * Returns whether the track has any soloed lanes.
 */
bool
track_has_soloed_lanes (const Track * const self)
{
  for (int i = 0; i < self->num_lanes; i++)
    {
      TrackLane * lane = self->lanes[i];
      if (track_lane_get_soloed (lane))
        return true;
    }

  return false;
}

/**
 * Returns the Track from the Project matching
 * \p name.
 *
 * @param name Name to search for.
 */
Track *
track_get_from_name (const char * name)
{
  g_return_val_if_fail (name, NULL);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (string_is_equal (track->name, name))
        {
          return track;
        }
    }

  g_return_val_if_reached (NULL);
}

Track *
track_find_by_name (const char * name)
{
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (g_strcmp0 (track->name, name) == 0)
        return track;
    }
  return NULL;
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
  size_t *         velocities_size,
  int              inside)
{
  if (
    track->type != TRACK_TYPE_MIDI
    && track->type != TRACK_TYPE_INSTRUMENT)
    return;

  for (int i = 0; i < track->num_lanes; i++)
    {
      TrackLane * lane = track->lanes[i];
      for (int j = 0; j < lane->num_regions; j++)
        {
          ZRegion * r = lane->regions[j];
          midi_region_get_velocities_in_range (
            r, start_pos, end_pos, velocities, num_velocities,
            velocities_size, inside);
        }
    }
}

/**
 * Verifies the identifiers on a live Track
 * (in the project, not a clone).
 *
 * @return True if pass.
 */
bool
track_validate (Track * self)
{
  g_return_val_if_fail (self, false);

  g_debug ("validating track '%s'...", self->name);

  if (self->channel)
    g_return_val_if_fail (
      self->channel->sends[0]->track_name_hash
        == self->channel->sends[0]->amount->id.track_name_hash,
      false);

  if (ZRYTHM_TESTING)
    {
      /* verify port identifiers */
      GPtrArray * ports = g_ptr_array_new ();
      track_append_ports (self, ports, F_INCLUDE_PLUGINS);
      AutomationTracklist * atl =
        track_get_automation_tracklist (self);
      unsigned int name_hash = track_get_name_hash (self);
      for (size_t i = 0; i < ports->len; i++)
        {
          Port * port = g_ptr_array_index (ports, i);
          g_return_val_if_fail (
            port->id.track_name_hash == name_hash, false);
          if (port->id.owner_type == PORT_OWNER_TYPE_PLUGIN)
            {
              PluginIdentifier * pid = &port->id.plugin_id;
              g_return_val_if_fail (
                pid->track_name_hash == name_hash, false);
              Plugin * pl = plugin_find (pid);
              g_return_val_if_fail (
                plugin_identifier_validate (pid), false);
              g_return_val_if_fail (
                plugin_identifier_validate (&pl->id), false);
              g_return_val_if_fail (
                plugin_identifier_is_equal (&pl->id, pid),
                false);
              if (pid->slot_type == PLUGIN_SLOT_INSTRUMENT)
                {
                  g_return_val_if_fail (
                    pl == self->channel->instrument, false);
                }
            }

          /* check that the automation track is
           * there */
          if (atl && port->id.flags & PORT_FLAG_AUTOMATABLE)
            {
              /*g_message ("checking %s", port->id.label);*/
              AutomationTrack * at =
                automation_track_find_from_port (
                  port, self, true);
              if (!at)
                {
                  char full_str[600];
                  port_get_full_designation (port, full_str);
                  g_critical (
                    "Could not find automation track "
                    "for port '%s'",
                    full_str);
                  return false;
                }
              g_return_val_if_fail (
                automation_track_find_from_port (
                  port, self, false),
                false);
              g_return_val_if_fail (port->at == at, false);

              automation_track_verify (at);
            }
        }
      object_free_w_func_and_null (g_ptr_array_unref, ports);
    }

  /* verify output and sends */
  if (self->channel)
    {
      Channel * ch = self->channel;
      Track *   out_track = channel_get_output_track (ch);
      g_return_val_if_fail (out_track != self, false);

      if (TRACK_CAN_BE_GROUP_TARGET (self))
        {
          group_target_track_validate (self);
        }

      /* verify plugins */
      Plugin * plugins[60];
      int      num_plugins =
        channel_get_plugins (self->channel, plugins);
      for (int i = 0; i < num_plugins; i++)
        {
          Plugin * pl = plugins[i];
          g_return_val_if_fail (plugin_validate (pl), false);
        }

      /* verify sends */
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          ChannelSend * send = ch->sends[i];
          channel_send_validate (send);
        }
    }

  /* verify tracklist identifiers */
  AutomationTracklist * atl =
    track_get_automation_tracklist (self);
  if (atl)
    {
      g_return_val_if_fail (
        automation_tracklist_validate (atl), false);
    }

  /* verify regions */
  for (int i = 0; i < self->num_lanes; i++)
    {
      TrackLane * lane = self->lanes[i];

      for (int j = 0; j < lane->num_regions; j++)
        {
          ZRegion * region = lane->regions[j];
          bool is_project = track_is_in_active_project (self);
          region_validate (region, is_project, 0);
        }
    }

  for (int i = 0; i < self->num_chord_regions; i++)
    {
      ZRegion * r = self->chord_regions[i];
      region_validate (
        r, track_is_in_active_project (self), 0);
    }

  g_debug ("done validating track '%s'", self->name);

  return true;
}

/**
 * Adds the track's folder parents to the given
 * array.
 *
 * @param prepend Whether to prepend instead of
 *   append.
 */
void
track_add_folder_parents (
  const Track * self,
  GPtrArray *   parents,
  bool          prepend)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * cur_track = TRACKLIST->tracks[i];
      if (!track_type_is_foldable (cur_track->type))
        continue;

      /* last position covered by the foldable
       * track cur_track */
      int last_covered_pos =
        cur_track->pos + (cur_track->size - 1);

      if (
        cur_track->pos < self->pos
        && self->pos <= last_covered_pos)
        {
          g_ptr_array_insert (
            parents, prepend ? 0 : -1, cur_track);
        }
    }
}

/**
 * Returns the closest foldable parent or NULL.
 */
Track *
track_get_direct_folder_parent (Track * track)
{
  GPtrArray * parents = g_ptr_array_new ();
  track_add_folder_parents (track, parents, true);
  Track * parent = NULL;
  if (parents->len > 0)
    {
      parent = g_ptr_array_index (parents, 0);
    }
  g_ptr_array_unref (parents);

  return parent;
}

/**
 * Remove the track from all folders.
 *
 * Used when deleting tracks.
 */
void
track_remove_from_folder_parents (Track * self)
{
  GPtrArray * parents = g_ptr_array_new ();
  track_add_folder_parents (self, parents, false);
  for (size_t j = 0; j < parents->len; j++)
    {
      Track * parent = g_ptr_array_index (parents, j);
      parent->size--;
    }
  g_ptr_array_unref (parents);
}

/**
 * Returns if the given TrackType can host the given
 * RegionType.
 */
int
track_type_can_host_region_type (
  const TrackType  tt,
  const RegionType rt)
{
  switch (rt)
    {
    case REGION_TYPE_MIDI:
      return tt == TRACK_TYPE_MIDI
             || tt == TRACK_TYPE_INSTRUMENT;
    case REGION_TYPE_AUDIO:
      return tt == TRACK_TYPE_AUDIO;
    case REGION_TYPE_AUTOMATION:
      return tt != TRACK_TYPE_CHORD && tt != TRACK_TYPE_MARKER;
    case REGION_TYPE_CHORD:
      return tt == TRACK_TYPE_CHORD;
    }
  g_return_val_if_reached (-1);
}

/**
 * Returns whether the track should be visible.
 *
 * Takes into account Track.visible and whether
 * any of the track's foldable parents are folded.
 */
NONNULL bool
track_get_should_be_visible (const Track * self)
{
  if (!self->visible || self->filtered)
    return false;

  GPtrArray * parents = g_ptr_array_new ();
  track_add_folder_parents (self, parents, false);
  for (size_t i = 0; i < parents->len; i++)
    {
      Track * parent = g_ptr_array_index (parents, i);
      if (!parent->visible || parent->folded)
        return false;
    }
  g_ptr_array_unref (parents);

  return true;
}

/**
 * Returns the full visible height (main height +
 * height of all visible automation tracks + height
 * of all visible lanes).
 */
double
track_get_full_visible_height (Track * const self)
{
  double height = self->main_height;

  if (self->lanes_visible)
    {
      for (int i = 0; i < self->num_lanes; i++)
        {
          TrackLane * lane = self->lanes[i];
          g_warn_if_fail (lane->height > 0);
          height += lane->height;
        }
    }
  if (self->automation_visible)
    {
      AutomationTracklist * atl =
        track_get_automation_tracklist (self);
      if (atl)
        {
          for (int i = 0; i < atl->num_ats; i++)
            {
              AutomationTrack * at = atl->ats[i];
              g_warn_if_fail (at->height > 0);
              if (at->visible)
                height += at->height;
            }
        }
    }
  return height;
}

bool
track_multiply_heights (
  Track * self,
  double  multiplier,
  bool    visible_only,
  bool    check_only)
{
  if (self->main_height * multiplier < TRACK_MIN_HEIGHT)
    return false;

  if (!check_only)
    {
      self->main_height *= multiplier;
    }

  if (!visible_only || self->lanes_visible)
    {
      for (int i = 0; i < self->num_lanes; i++)
        {
          TrackLane * lane = self->lanes[i];

          if (lane->height * multiplier < TRACK_MIN_HEIGHT)
            {
              return false;
            }

          if (!check_only)
            {
              lane->height *= multiplier;
            }
        }
    }
  if (!visible_only || self->automation_visible)
    {
      AutomationTracklist * atl =
        track_get_automation_tracklist (self);
      if (atl)
        {
          for (int i = 0; i < atl->num_ats; i++)
            {
              AutomationTrack * at = atl->ats[i];

              if (visible_only && !at->visible)
                continue;

              if (at->height * multiplier < TRACK_MIN_HEIGHT)
                {
                  return false;
                }

              if (!check_only)
                {
                  at->height *= multiplier;
                }
            }
        }
    }

  return true;
}

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
void
track_set_soloed (
  Track * self,
  bool    solo,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events)
{
#if 0
  if (self->type == TRACK_TYPE_FOLDER)
    {
      /* TODO */
      return;
    }
#endif

  if (auto_select)
    {
      track_select (self, F_SELECT, F_EXCLUSIVE, fire_events);
    }

  if (trigger_undo)
    {
      /* this is only supported if the fader track
       * is the only track selected */
      g_return_if_fail (
        TRACKLIST_SELECTIONS->num_tracks == 1
        && TRACKLIST_SELECTIONS->tracks[0] == self);

      GError * err = NULL;
      bool ret = tracklist_selections_action_perform_edit_solo (
        TRACKLIST_SELECTIONS, solo, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track soloed"));
          return;
        }
    }
  else
    {
      fader_set_soloed (
        self->channel->fader, solo, fire_events);
    }
}

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
void
track_set_listened (
  Track * self,
  bool    listen,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events)
{
  if (auto_select)
    {
      track_select (self, F_SELECT, F_EXCLUSIVE, fire_events);
    }

  if (trigger_undo)
    {
      /* this is only supported if the fader track
       * is the only track selected */
      g_return_if_fail (
        TRACKLIST_SELECTIONS->num_tracks == 1
        && TRACKLIST_SELECTIONS->tracks[0] == self);

      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_listen (
          TRACKLIST_SELECTIONS, listen, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track listened"));
          return;
        }
    }
  else
    {
      fader_set_listened (
        self->channel->fader, listen, fire_events);
    }
}

/**
 * Writes the track to the given MIDI file.
 *
 * @param use_track_pos Whether to use the track
 *   position in the MIDI data. The track will be
 *   set to 1 if false.
 * @param events Track events, if not using lanes
 *   as tracks or using track position.
 */
void
track_write_to_midi_file (
  const Track * self,
  MIDI_FILE *   mf,
  MidiEvents *  events,
  bool          lanes_as_tracks,
  bool          use_track_pos)
{
  g_return_if_fail (track_type_has_piano_roll (self->type));

  bool own_events = false;
  int  midi_track_pos = self->pos;
  if (!lanes_as_tracks && use_track_pos)
    {
      g_return_if_fail (!events);
      midiTrackAddText (
        mf, self->pos, textTrackName, self->name);
      events = midi_events_new ();
      own_events = true;
    }

  for (int i = 0; i < self->num_lanes; i++)
    {
      TrackLane * lane = self->lanes[i];

      track_lane_write_to_midi_file (
        lane, mf, events, lanes_as_tracks, use_track_pos);
    }

  if (own_events)
    {
      midi_events_write_to_midi_file (
        events, mf, midi_track_pos);
      object_free_w_func_and_null (midi_events_free, events);
    }
}

/**
 * Returns if Track is in TracklistSelections.
 */
int
track_is_selected (Track * self)
{
  if (tracklist_selections_contains_track (
        TRACKLIST_SELECTIONS, self))
    return 1;

  return 0;
}

bool
track_contains_uninstantiated_plugin (const Track * const self)
{
  GPtrArray * arr = g_ptr_array_new_full (20, NULL);
  track_get_plugins (self, arr);

  bool ret = false;
  for (size_t i = 0; i < arr->len; i++)
    {
      Plugin * pl = g_ptr_array_index (arr, i);

      if (pl->instantiation_failed)
        {
          ret = true;
          break;
        }
    }

  g_ptr_array_unref (arr);

  return ret;
}

/**
 * Returns the last region in the track, or NULL.
 *
 * FIXME cache.
 */
ZRegion *
track_get_last_region (Track * track)
{
  int              i, j;
  ZRegion *        last_region = NULL, *r;
  ArrangerObject * r_obj;
  Position         tmp;
  position_init (&tmp);

  if (
    track->type == TRACK_TYPE_AUDIO
    || track->type == TRACK_TYPE_INSTRUMENT)
    {
      TrackLane * lane;
      for (i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              r_obj = (ArrangerObject *) r;
              if (position_is_after (&r_obj->end_pos, &tmp))
                {
                  last_region = r;
                  position_set_to_pos (&tmp, &r_obj->end_pos);
                }
            }
        }
    }

  AutomationTracklist * atl = &track->automation_tracklist;
  AutomationTrack *     at;
  for (i = 0; i < atl->num_ats; i++)
    {
      at = atl->ats[i];
      r = automation_track_get_last_region (at);
      r_obj = (ArrangerObject *) r;

      if (!r)
        continue;

      if (position_is_after (&r_obj->end_pos, &tmp))
        {
          last_region = r;
          position_set_to_pos (&tmp, &r_obj->end_pos);
        }
    }

  return last_region;
}

/**
 * Generates automatables for the track.
 *
 * Should be called as soon as the track is
 * created.
 */
void
track_generate_automation_tracks (Track * track)
{
  g_message (
    "generating automation tracks for '%s'", track->name);

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at;

  if (track_type_has_channel (track->type))
    {
      Channel * ch = track->channel;

      /* -- fader -- */

      /* volume */
      at = automation_track_new (ch->fader->amp);
      automation_tracklist_add_at (atl, at);
      at->created = 1;
      at->visible = 1;

      /* balance */
      at = automation_track_new (ch->fader->balance);
      automation_tracklist_add_at (atl, at);

      /* mute */
      at = automation_track_new (ch->fader->mute);
      automation_tracklist_add_at (atl, at);

      /* --- end fader --- */

      /* sends */
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          at = automation_track_new (ch->sends[i]->amount);
          automation_tracklist_add_at (atl, at);
        }
    }

  if (track_type_has_piano_roll (track->type))
    {
      /* midi automatables */
      for (int i = 0; i < 16; i++)
        {
          Port * cc = NULL;
          for (int j = 0; j < 128; j++)
            {
              cc = track->processor->midi_cc[i * 128 + j];
              at = automation_track_new (cc);
              automation_tracklist_add_at (atl, at);
            }

          cc = track->processor->pitch_bend[i];
          at = automation_track_new (cc);
          automation_tracklist_add_at (atl, at);

          cc = track->processor->poly_key_pressure[i];
          at = automation_track_new (cc);
          automation_tracklist_add_at (atl, at);

          cc = track->processor->channel_pressure[i];
          at = automation_track_new (cc);
          automation_tracklist_add_at (atl, at);
        }
    }

  switch (track->type)
    {
    case TRACK_TYPE_TEMPO:
      /* create special BPM and time sig automation
     * tracks for tempo track */
      at = automation_track_new (track->bpm_port);
      at->created = true;
      at->visible = true;
      automation_tracklist_add_at (atl, at);
      at = automation_track_new (track->beats_per_bar_port);
      automation_tracklist_add_at (atl, at);
      at = automation_track_new (track->beat_unit_port);
      automation_tracklist_add_at (atl, at);
      break;
    case TRACK_TYPE_MODULATOR:
      for (int i = 0; i < track->num_modulator_macros; i++)
        {
          at = automation_track_new (
            track->modulator_macros[i]->macro);
          if (i == 0)
            {
              at->created = true;
              at->visible = true;
            }
          automation_tracklist_add_at (atl, at);
        }
      break;
    case TRACK_TYPE_AUDIO:
      at =
        automation_track_new (track->processor->output_gain);
      break;
    default:
      break;
    }

  g_message ("done");
}

/**
 * Wrapper.
 */
void
track_setup (Track * track)
{
#define SETUP_TRACK(uc, sc) \
  case TRACK_TYPE_##uc: \
    sc##_track_setup (track); \
    break;

  switch (track->type)
    {
      SETUP_TRACK (INSTRUMENT, instrument);
      SETUP_TRACK (MASTER, master);
      SETUP_TRACK (AUDIO, audio);
      SETUP_TRACK (AUDIO_BUS, audio_bus);
      SETUP_TRACK (AUDIO_GROUP, audio_group);
    case TRACK_TYPE_CHORD:
    /* TODO */
    default:
      break;
    }

#undef SETUP_TRACK
}

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
  GError **         error)
{
  if (region->id.type == REGION_TYPE_AUTOMATION)
    {
      track = automation_track_get_track (at);
    }
  g_return_val_if_fail (IS_TRACK (track), false);
  g_return_val_if_fail (
    region_validate (region, false, 0), false);
  g_return_val_if_fail (
    track_type_can_have_region_type (
      track->type, region->id.type),
    false);

  if (gen_name)
    {
      region_gen_name (region, NULL, at, track);
    }

  g_return_val_if_fail (region->name, false);
  g_message (
    "inserting region '%s' to track '%s' "
    "at lane %d (idx %d)",
    region->name, track->name, lane_pos, idx);

  int add_lane = 0, add_at = 0, add_chord = 0;
  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      add_lane = 1;
      break;
    case REGION_TYPE_AUDIO:
      add_lane = 1;
      break;
    case REGION_TYPE_AUTOMATION:
      add_at = 1;
      break;
    case REGION_TYPE_CHORD:
      add_chord = 1;
      break;
    }

  if (add_lane)
    {
      /* enable extra lane if necessary */
      track_create_missing_lanes (track, lane_pos);

      g_return_val_if_fail (track->lanes[lane_pos], false);
      if (idx == -1)
        {
          track_lane_add_region (
            track->lanes[lane_pos], region);
        }
      else
        {
          track_lane_insert_region (
            track->lanes[lane_pos], region, idx);
        }
      g_return_val_if_fail (region->id.idx >= 0, false);
    }

  if (add_at)
    {
      if (idx == -1)
        {
          automation_track_add_region (at, region);
        }
      else
        {
          automation_track_insert_region (at, region, idx);
        }
    }

  if (add_chord)
    {
      g_return_val_if_fail (track == P_CHORD_TRACK, false);

      if (idx == -1)
        {
          chord_track_insert_chord_region (
            track, region, track->num_chord_regions);
        }
      else
        {
          chord_track_insert_chord_region (track, region, idx);
        }
    }

  /* write clip if audio region */
  if (
    region->id.type == REGION_TYPE_AUDIO
    && !track_is_auditioner (track))
    {
      AudioClip * clip = audio_region_get_clip (region);
      GError *    err = NULL;
      bool        success = audio_clip_write_to_pool (
        clip, false, F_NOT_BACKUP, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s",
            "Failed to write audio region clip to pool");
          return false;
        }
    }

  g_message ("inserted:");
  region_print (region);

  if (fire_events)
    {
      EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, region);

      if (add_lane)
        {
          EVENTS_PUSH (ET_TRACK_LANE_ADDED, NULL);
        }
    }

  return true;
}

/**
 * Creates missing TrackLane's until pos.
 *
 * @return Whether a new lane was created.
 */
bool
track_create_missing_lanes (Track * self, const int pos)
{
  if (self->block_auto_creation_and_deletion)
    return false;

  bool ret = false;
  while (self->num_lanes < pos + 2)
    {
      track_add_lane (self, 0);
      ret = true;
    }

  return ret;
}

/**
 * Removes the empty last lanes of the Track
 * (except the last one).
 */
void
track_remove_empty_last_lanes (Track * self)
{
  g_return_if_fail (self);

  if (self->block_auto_creation_and_deletion)
    return;

  /* if currently have a temporary last lane created skip
   * removing */
  if (self->last_lane_created > 0)
    return;

  g_message ("removing empty last lanes from %s", self->name);
  int removed = 0;
  for (int i = self->num_lanes - 1; i >= 1; i--)
    {
      TrackLane * lane = self->lanes[i];
      TrackLane * prev_lane = self->lanes[i - 1];
      g_return_if_fail (lane && prev_lane);

      if (lane->num_regions > 0)
        break;

      if (lane->num_regions == 0 && prev_lane->num_regions == 0)
        {
          g_message ("removing lane %d", i);
          self->num_lanes--;
          object_free_w_func_and_null (
            track_lane_free, self->lanes[i]);
          self->lanes[i] = NULL;
          removed = 1;
        }
    }

  if (removed)
    {
      EVENTS_PUSH (ET_TRACK_LANE_REMOVED, NULL);
    }
}

/**
 * Updates the track's children.
 *
 * Used when changing track positions.
 */
void
track_update_children (Track * self)
{
  unsigned int name_hash = track_get_name_hash (self);
  for (int i = 0; i < self->num_children; i++)
    {
      Track * child = tracklist_find_track_by_name_hash (
        TRACKLIST, self->children[i]);
      g_warn_if_fail (
        IS_TRACK (child)
        && child->out_signal_type == self->in_signal_type);
      child->channel->output_name_hash = name_hash;
      g_debug (
        "%s: setting output of track %s [%d] to "
        "%s [%d]",
        __func__, child->name, child->pos, self->name,
        self->pos);
    }
}

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
track_freeze (Track * self, bool freeze, GError ** error)
{
  g_message (
    "%sfreezing %s...", freeze ? "" : "un", self->name);

  if (freeze)
    {
      ExportSettings * settings = export_settings_new ();
      track_mark_for_bounce (
        self, F_BOUNCE, F_MARK_REGIONS, F_NO_MARK_CHILDREN,
        F_NO_MARK_PARENTS);
      settings->mode = EXPORT_MODE_TRACKS;
      export_settings_set_bounce_defaults (
        settings, EXPORT_FORMAT_WAV, NULL, self->name);

      EngineState state;
      GPtrArray * conns =
        exporter_prepare_tracks_for_export (settings, &state);

      /* start exporting in a new thread */
      GThread * thread = g_thread_new (
        "bounce_thread",
        (GThreadFunc) exporter_generic_export_thread,
        settings);

      /* create a progress dialog and block */
      ExportProgressDialogWidget * progress_dialog =
        export_progress_dialog_widget_new (
          settings, true, false, F_CANCELABLE);
      gtk_window_set_transient_for (
        GTK_WINDOW (progress_dialog),
        GTK_WINDOW (MAIN_WINDOW));
      z_gtk_dialog_run (GTK_DIALOG (progress_dialog), true);

      g_thread_join (thread);

      exporter_post_export (settings, conns, &state);

      /* assert exporting is finished */
      g_return_val_if_fail (!AUDIO_ENGINE->exporting, false);

      if (
        progress_info_get_completion_type (
          settings->progress_info)
        == PROGRESS_COMPLETED_SUCCESS)
        {
          /* move the temporary file to the pool */
          AudioClip * clip =
            audio_clip_new_from_file (settings->file_uri);
          audio_pool_add_clip (AUDIO_POOL, clip);
          GError * err = NULL;
          bool     success = audio_clip_write_to_pool (
            clip, F_NO_PARTS, F_NOT_BACKUP, &err);
          if (!success)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err,
                "Failed to write frozen audio for track '%s' to pool",
                self->name);
              return false;
            }
          self->pool_id = clip->pool_id;
        }

      if (g_file_test (
            settings->file_uri, G_FILE_TEST_IS_REGULAR))
        {
          io_remove (settings->file_uri);
        }

      export_settings_free (settings);
    }
  else
    {
      /* FIXME */
      /*audio_pool_remove_clip (*/
      /*AUDIO_POOL, self->pool_id, true);*/
    }

  self->frozen = freeze;
  EVENTS_PUSH (ET_TRACK_FREEZE_CHANGED, self);

  return true;
}

/**
 * Wrapper over channel_add_plugin() and
 * modulator_track_insert_modulator().
 *
 * @param instantiate_plugin Whether to attempt to
 *   instantiate the plugin.
 */
void
track_insert_plugin (
  Track *        self,
  Plugin *       pl,
  PluginSlotType slot_type,
  int            slot,
  bool           instantiate_plugin,
  bool           replacing_plugin,
  bool           moving_plugin,
  bool           confirm,
  bool           gen_automatables,
  bool           recalc_graph,
  bool           fire_events)
{
  g_return_if_fail (
    plugin_identifier_validate_slot_type_slot_combo (
      slot_type, slot));

  if (slot_type == PLUGIN_SLOT_MODULATOR)
    {
      modulator_track_insert_modulator (
        self, slot, pl, replacing_plugin, confirm,
        gen_automatables, recalc_graph, fire_events);
    }
  else
    {
      channel_add_plugin (
        self->channel, slot_type, slot, pl, confirm,
        moving_plugin, gen_automatables, recalc_graph,
        fire_events);
    }

  if (!pl->instantiated && !pl->instantiation_failed)
    {
      GError * err = NULL;
      int      ret = plugin_instantiate (pl, NULL, &err);
      if (ret != 0)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to instantiate plugin"));
        }
    }
}

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
  bool           recalc_graph)
{
  if (slot_type == PLUGIN_SLOT_MODULATOR)
    {
      modulator_track_remove_modulator (
        self, slot, replacing_plugin, deleting_plugin,
        deleting_track, recalc_graph);
    }
  else
    {
      channel_remove_plugin (
        self->channel, slot_type, slot, moving_plugin,
        deleting_plugin, deleting_track, recalc_graph);
    }
}

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
track_disconnect (Track * self, bool remove_pl, bool recalc_graph)
{
  g_message (
    "disconnecting track '%s' (%d)...", self->name, self->pos);

  self->disconnecting = true;

  /* if this is a group track and has children,
   * remove them */
  if (
    track_is_in_active_project (self)
    && !track_is_auditioner (self)
    && TRACK_CAN_BE_GROUP_TARGET (self))
    {
      group_target_track_remove_all_children (
        self, F_DISCONNECT, F_NO_RECALC_GRAPH,
        F_NO_PUBLISH_EVENTS);
    }

  /* disconnect all ports and free buffers */
  GPtrArray * ports = g_ptr_array_new ();
  track_append_ports (self, ports, F_INCLUDE_PLUGINS);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      if (
        !IS_PORT (port)
        || port_is_in_active_project (port)
             != track_is_in_active_project (self))
        {
          g_critical ("invalid port");
          object_free_w_func_and_null (
            g_ptr_array_unref, ports);
          return;
        }

      port_disconnect_all (port);
      port_free_bufs (port);
    }
  object_free_w_func_and_null (g_ptr_array_unref, ports);

  if (
    track_is_in_active_project (self)
    && !track_is_auditioner (self))
    {
      /* disconnect from folders */
      track_remove_from_folder_parents (self);
    }

  if (recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  if (track_type_has_channel (self->type))
    {
      channel_disconnect (self->channel, remove_pl);
    }

  self->disconnecting = false;

  g_debug ("done");
}

/**
 * Set track lanes visible and fire events.
 */
void
track_set_lanes_visible (Track * track, const int visible)
{
  track->lanes_visible = visible;

  EVENTS_PUSH (ET_TRACK_LANES_VISIBILITY_CHANGED, track);
}

/**
 * Set automation visible and fire events.
 */
void
track_set_automation_visible (Track * self, const bool visible)
{
  self->automation_visible = visible;

  if (visible)
    {
      /* if no visible automation tracks, set the
       * first one visible */
      AutomationTracklist * atl =
        track_get_automation_tracklist (self);
      g_return_if_fail (atl);
      int num_visible =
        automation_tracklist_get_num_visible (atl);

      if (num_visible == 0)
        {
          AutomationTrack * at =
            automation_tracklist_get_first_invisible_at (atl);
          if (at)
            {
              at->created = true;
              at->visible = true;
            }
          else
            {
              g_message (
                "no automation tracks found for %s",
                self->name);
            }
        }
    }

  EVENTS_PUSH (ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, self);
}

/**
 * Unselects all arranger objects in the track.
 */
void
track_unselect_all (Track * self)
{
  if (track_is_auditioner (self))
    return;

  /* unselect lane regions */
  for (int i = 0; i < self->num_lanes; i++)
    {
      TrackLane * lane = self->lanes[i];
      track_lane_unselect_all (lane);
    }

  /* unselect automation regions */
  AutomationTracklist * atl =
    track_get_automation_tracklist (self);
  if (atl)
    {
      automation_tracklist_unselect_all (atl);
    }
}

/**
 * Removes all arranger objects recursively.
 */
void
track_clear (Track * self)
{
  g_return_if_fail (IS_TRACK (self));

  /* remove lane regions */
  for (int i = self->num_lanes - 1; i >= 0; i--)
    {
      TrackLane * lane = self->lanes[i];
      track_lane_clear (lane);
    }

  /* remove automation regions */
  AutomationTracklist * atl =
    track_get_automation_tracklist (self);
  if (atl)
    {
      automation_tracklist_clear (atl);
    }
}

/**
 * Only removes the region from the track.
 *
 * @pararm free Also free the Region.
 */
void
track_remove_region (
  Track *   self,
  ZRegion * region,
  bool      fire_events,
  bool      free)
{
  g_return_if_fail (IS_TRACK (self) && IS_REGION (region));

  g_message ("removing region from track '%s':", self->name);
  if (!track_is_auditioner (self))
    region_print (region);

  /* check if region type matches track type */
  g_return_if_fail (track_type_can_have_region_type (
    self->type, region->id.type));

  if (!track_is_auditioner (self))
    region_disconnect (region);

  g_warn_if_fail (region->id.lane_pos >= 0);

  bool has_lane = false;
  if (region_type_has_lane (region->id.type))
    {
      has_lane = true;
      TrackLane * lane = self->lanes[region->id.lane_pos];
      track_lane_remove_region (lane, region);
    }
  else if (region->id.type == REGION_TYPE_CHORD)
    {
      chord_track_remove_region (self, region);
    }
  else if (region->id.type == REGION_TYPE_AUTOMATION)
    {
      AutomationTracklist * atl = &self->automation_tracklist;
      for (int i = 0; i < atl->num_ats; i++)
        {
          AutomationTrack * at = atl->ats[i];
          if (at->index == region->id.at_idx)
            {
              automation_track_remove_region (at, region);
            }
        }
    }

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW && !track_is_auditioner (self))
    {
      ArrangerObject * obj = (ArrangerObject *) region;
      ArrangerWidget * arranger =
        arranger_object_get_arranger (obj);
      if (arranger->hovered_object == obj)
        {
          arranger->hovered_object = NULL;
        }
    }

  if (free)
    {
      arranger_object_free ((ArrangerObject *) region);
    }

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_REMOVED,
        ARRANGER_OBJECT_TYPE_REGION);

      if (has_lane)
        {
          track_remove_empty_last_lanes (self);
        }
    }
}

/**
 * Returns the automation tracklist if the track type has one,
 * or NULL if it doesn't (like chord tracks).
 */
AutomationTracklist *
track_get_automation_tracklist (Track * const track)
{
  g_return_val_if_fail (IS_TRACK (track), NULL);

  switch (track->type)
    {
    case TRACK_TYPE_MARKER:
    case TRACK_TYPE_FOLDER:
      break;
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_AUDIO_GROUP:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_MIDI_GROUP:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_MIDI:
    case TRACK_TYPE_TEMPO:
    case TRACK_TYPE_MODULATOR:
      return &track->automation_tracklist;
    default:
      g_warn_if_reached ();
      break;
    }

  return NULL;
}

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
  bool             include_region_end)
{
  int i, j;

  if (
    track->type == TRACK_TYPE_INSTRUMENT
    || track->type == TRACK_TYPE_AUDIO
    || track->type == TRACK_TYPE_MIDI)
    {
      TrackLane *      lane;
      ZRegion *        r;
      ArrangerObject * r_obj;
      for (i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              r_obj = (ArrangerObject *) r;
              if (
                pos->frames >= r_obj->pos.frames
                && pos->frames
                     < r_obj->end_pos.frames
                         + (include_region_end ? 1 : 0))
                {
                  return r;
                }
            }
        }
    }
  else if (track->type == TRACK_TYPE_CHORD)
    {
      ZRegion *        r;
      ArrangerObject * r_obj;
      for (j = 0; j < track->num_chord_regions; j++)
        {
          r = track->chord_regions[j];
          r_obj = (ArrangerObject *) r;
          if (
            position_is_after_or_equal (pos, &r_obj->pos)
            && pos->frames
                 < r_obj->end_pos.frames
                     + (include_region_end ? 1 : 0))
            {
              return r;
            }
        }
    }

  return NULL;
}

const char *
track_stringize_type (TrackType type)
{
  return _ (track_type_strings[type].str);
}

/**
 * Returns the Fader (if applicable).
 *
 * @param post_fader True to get post fader,
 *   false to get pre fader.
 */
Fader *
track_get_fader (Track * self, bool post_fader)
{
  Channel * ch = track_get_channel (self);
  if (ch)
    {
      if (post_fader)
        {
          return ch->fader;
        }
      else
        {
          return ch->prefader;
        }
    }

  return NULL;
}

/**
 * Returns the FaderType corresponding to the given
 * Track.
 */
FaderType
track_get_fader_type (const Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_MIDI:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_MIDI_GROUP:
      return FADER_TYPE_MIDI_CHANNEL;
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO_GROUP:
      return FADER_TYPE_AUDIO_CHANNEL;
    case TRACK_TYPE_MARKER:
      return FADER_TYPE_NONE;
    default:
      g_return_val_if_reached (FADER_TYPE_NONE);
    }
}

/**
 * Returns the prefader type
 * corresponding to the given Track.
 */
FaderType
track_type_get_prefader_type (TrackType type)
{
  switch (type)
    {
    case TRACK_TYPE_MIDI:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_MIDI_GROUP:
      return FADER_TYPE_MIDI_CHANNEL;
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO_GROUP:
      return FADER_TYPE_AUDIO_CHANNEL;
    case TRACK_TYPE_MARKER:
      return FADER_TYPE_NONE;
    default:
      g_return_val_if_reached (FADER_TYPE_NONE);
    }
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
track_update_positions (
  Track * self,
  bool    from_ticks,
  bool    bpm_change)
{
  int i;
  for (i = 0; i < self->num_lanes; i++)
    {
      track_lane_update_positions (
        self->lanes[i], from_ticks, bpm_change);
    }
  for (i = 0; i < self->num_chord_regions; i++)
    {
      arranger_object_update_positions (
        (ArrangerObject *) self->chord_regions[i], from_ticks,
        bpm_change, NULL);
    }
  for (i = 0; i < self->num_scales; i++)
    {
      arranger_object_update_positions (
        (ArrangerObject *) self->scales[i], from_ticks,
        bpm_change, NULL);
    }
  for (i = 0; i < self->num_markers; i++)
    {
      arranger_object_update_positions (
        (ArrangerObject *) self->markers[i], from_ticks,
        bpm_change, NULL);
    }

  automation_tracklist_update_positions (
    &self->automation_tracklist, from_ticks, bpm_change);
}

/**
 * Wrapper for audio and MIDI/instrument tracks
 * to fill in MidiEvents or StereoPorts from the
 * timeline data.
 *
 * @note The engine splits the cycle so transport
 *   loop related logic is not needed.
 *
 * @param stereo_ports StereoPorts to fill.
 * @param midi_events MidiEvents to fill (from
 *   Piano Roll Port for example).
 */
void
track_fill_events (
  const Track *                       self,
  const EngineProcessTimeInfo * const time_nfo,
  MidiEvents *                        midi_events,
  StereoPorts *                       stereo_ports)
{
  if (!track_is_auditioner (self) && !TRANSPORT_IS_ROLLING)
    {
      return;
    }

  const unsigned_frame_t g_end_frames =
    time_nfo->g_start_frame + time_nfo->nframes;

  if (midi_events)
    {
      /* FIXME don't block */
      zix_sem_wait (&midi_events->access_sem);
    }

#if 0
  g_message (
    "%s: TRACK %s STARTING from %ld, "
    "local start frame %u, nframes %u",
    __func__, self->name, time_nfo->g_start_frame,
    time_nfo->local_offset, time_nfo->nframes);
#endif

  TrackType tt = self->type;

  bool use_caches = !track_is_auditioner (self);

  TrackLane ** lanes =
    use_caches ? self->lane_snapshots : self->lanes;
  int num_lanes =
    use_caches ? self->num_lane_snapshots : self->num_lanes;
  ZRegion ** chord_regions =
    use_caches
      ? self->chord_region_snapshots
      : self->chord_regions;
  int num_chord_regions =
    use_caches
      ? self->num_chord_region_snapshots
      : self->num_chord_regions;

  /* go through each lane */
  const int num_loops =
    (tt == TRACK_TYPE_CHORD ? 1 : num_lanes);
  for (int j = 0; j < num_loops; j++)
    {
      TrackLane * lane = NULL;
      if (tt != TRACK_TYPE_CHORD)
        {
          lane = lanes[j];
          g_return_if_fail (lane);
        }

      /* go through each region */
      const int num_regions =
        (tt == TRACK_TYPE_CHORD
           ? num_chord_regions
           : lane->num_regions);
      for (int i = 0; i < num_regions; i++)
        {
          ZRegion * r =
            tt == TRACK_TYPE_CHORD
              ? chord_regions[i]
              : lane->regions[i];
          ArrangerObject * r_obj = (ArrangerObject *) r;
          g_return_if_fail (IS_REGION (r));

          /* skip region if muted */
          if (arranger_object_get_muted (r_obj, true))
            {
              continue;
            }

          /* skip if in bounce mode and the
           * region should not be bounced */
          if (
            AUDIO_ENGINE->bounce_mode != BOUNCE_OFF
            && (!r->bounce || !self->bounce))
            {
              continue;
            }

          /* skip if region is not hit
           * (inclusive of its last point) */
          if (
            !region_is_hit_by_range (
              r, (signed_frame_t) time_nfo->g_start_frame,
              (signed_frame_t) (midi_events ? g_end_frames : (g_end_frames - 1)),
              F_INCLUSIVE))
            {
              continue;
            }

          signed_frame_t num_frames_to_process = MIN (
            r_obj->end_pos.frames
              - (signed_frame_t) time_nfo->g_start_frame,
            (signed_frame_t) time_nfo->nframes);
          nframes_t frames_processed = 0;

          while (num_frames_to_process > 0)
            {
              unsigned_frame_t cur_g_start_frame =
                time_nfo->g_start_frame + frames_processed;
              nframes_t cur_local_start_frame =
                time_nfo->local_offset + frames_processed;

              bool is_end_loop;
              signed_frame_t
                cur_num_frames_till_next_r_loop_or_end;
              region_get_frames_till_next_loop_or_end (
                r, (signed_frame_t) cur_g_start_frame,
                &cur_num_frames_till_next_r_loop_or_end,
                &is_end_loop);

#if 0
              g_message (
                "%s: cur num frames till next r "
                "loop or end %ld, "
                "num_frames_to_process %ld, "
                "cur local start frame %u",
                __func__, cur_num_frames_till_next_r_loop_or_end,
                num_frames_to_process,
                cur_local_start_frame);
#endif

              /* whether we need a note off */
              bool need_note_off =
                (cur_num_frames_till_next_r_loop_or_end
                 < num_frames_to_process)
                || (cur_num_frames_till_next_r_loop_or_end == num_frames_to_process && !is_end_loop)
                ||
                /* region end */
                ((signed_frame_t) time_nfo->g_start_frame
                   + num_frames_to_process
                 == r_obj->end_pos.frames)
                ||
                /* transport end */
                (TRANSPORT_IS_LOOPING
                 && (signed_frame_t) time_nfo->g_start_frame
                        + num_frames_to_process
                      == TRANSPORT->loop_end_pos.frames);

              /* number of frames to process this
               * time */
              cur_num_frames_till_next_r_loop_or_end = MIN (
                cur_num_frames_till_next_r_loop_or_end,
                num_frames_to_process);

              const EngineProcessTimeInfo nfo = {
                .g_start_frame = cur_g_start_frame,
                .local_offset = cur_local_start_frame,
                .nframes = cur_num_frames_till_next_r_loop_or_end
              };

              if (midi_events)
                {
                  midi_region_fill_midi_events (
                    r, &nfo, need_note_off, midi_events);
                }
              else if (stereo_ports)
                {
                  audio_region_fill_stereo_ports (
                    r, &nfo, stereo_ports);
                }

              frames_processed +=
                cur_num_frames_till_next_r_loop_or_end;
              num_frames_to_process -=
                cur_num_frames_till_next_r_loop_or_end;
            } /* end while frames left */
        }
    }

#if 0
  g_message ("TRACK %s ENDING", self->name);
#endif

  if (midi_events)
    {
      midi_events_clear_duplicates (midi_events, F_QUEUED);

      /* sort events */
      midi_events_sort (midi_events, F_QUEUED);

      zix_sem_post (&midi_events->access_sem);
    }
}

/**
 * Wrapper to get the track name.
 */
const char *
track_get_name (Track * track)
{
#if 0
  if (DEBUGGING)
    {
      return
        g_strdup_printf (
          "%s%s",
          track_is_selected (track) ? "* " : "",
          track->name);
    }
#endif
  return track->name;
}

/**
 * Internally called by
 * track_set_name_with_action().
 */
bool
track_set_name_with_action_full (
  Track *      track,
  const char * name)
{
  g_return_val_if_fail (IS_TRACK (track), false);

  GError * err = NULL;
  bool ret = tracklist_selections_action_perform_edit_rename (
    track, PORT_CONNECTIONS_MGR, name, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to rename track"));
      return false;
    }
  return true;
}

/**
 * Setter to be used by the UI to create an
 * undoable action.
 */
void
track_set_name_with_action (Track * track, const char * name)
{
  track_set_name_with_action_full (track, name);
}

static void
add_region_if_in_range (
  Track *    track,
  Position * p1,
  Position * p2,
  ZRegion ** regions,
  int *      count,
  ZRegion *  region)
{
  ArrangerObject * r_obj = (ArrangerObject *) region;

  if (!p1 && !p2)
    {
      regions[(*count)++] = region;
      return;
    }
  else
    {
      g_return_if_fail (p1 && p2);
    }

  /* start inside */
  if (
    (position_is_before_or_equal (p1, &r_obj->pos)
     && position_is_after (p2, &r_obj->pos))
    ||
    /* end inside */
    (position_is_before_or_equal (p1, &r_obj->end_pos)
     && position_is_after (p2, &r_obj->end_pos))
    ||
    /* start before and end after */
    (position_is_before_or_equal (&r_obj->pos, p1)
     && position_is_after (&r_obj->end_pos, p2)))
    {
      regions[(*count)++] = region;
    }
}

/**
 * Returns all the regions inside the given range,
 * or all the regions if both @ref p1 and @ref p2
 * are NULL.
 *
 * @return The number of regions returned.
 */
int
track_get_regions_in_range (
  Track *    track,
  Position * p1,
  Position * p2,
  ZRegion ** regions)
{
  int count = 0;

  if (
    track->type == TRACK_TYPE_INSTRUMENT
    || track->type == TRACK_TYPE_AUDIO
    || track->type == TRACK_TYPE_MIDI)
    {
      for (int i = 0; i < track->num_lanes; i++)
        {
          TrackLane * lane = track->lanes[i];

          for (int j = 0; j < lane->num_regions; j++)
            {
              ZRegion * r = lane->regions[j];
              add_region_if_in_range (
                track, p1, p2, regions, &count, r);
            }
        }
    }
  else if (track->type == TRACK_TYPE_CHORD)
    {
      for (int j = 0; j < track->num_chord_regions; j++)
        {
          ZRegion * r = track->chord_regions[j];
          add_region_if_in_range (
            track, p1, p2, regions, &count, r);
        }
    }

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  if (atl)
    {
      for (int i = 0; i < atl->num_ats; i++)
        {
          AutomationTrack * at = atl->ats[i];

          for (int j = 0; j < at->num_regions; j++)
            {
              ZRegion * r = at->regions[j];
              add_region_if_in_range (
                track, p1, p2, regions, &count, r);
            }
        }
    }

  return count;
}

/**
 * Returns a unique name for a new track based on
 * the given name.
 */
char *
track_get_unique_name (Track * track_to_skip, const char * _name)
{
  /* add enough space for brackets and number
   * inside brackets */
  char name[strlen (_name) + 40];
  strcpy (name, _name);
  while (!tracklist_track_name_is_unique (
    TRACKLIST, name, track_to_skip))
    {
      char name_without_num[strlen (name) + 1];
      int  ending_num = string_get_int_after_last_space (
        name, name_without_num);
      if (ending_num == -1)
        {
          /* append 1 at the end */
          strcat (name, " 1");
        }
      else
        {
          /* add 1 to the number at the end */
          sprintf (
            name, "%s %d", name_without_num, ending_num + 1);
        }
    }

  return g_strdup (name);
}

/**
 * Setter for the track name.
 *
 * If a track with that name already exists, it
 * adds a number at the end.
 *
 * Must only be called from the GTK thread.
 */
void
track_set_name (Track * self, const char * name, bool pub_events)
{
  char * new_name = track_get_unique_name (self, name);
  g_return_if_fail (new_name);

  unsigned int old_hash =
    self->name ? track_get_name_hash (self) : 0;

  if (self->name)
    {
      old_hash = track_get_name_hash (self);
      g_free (self->name);
    }
  self->name = new_name;

  unsigned int new_hash = track_get_name_hash (self);

  if (old_hash != 0)
    {
      for (int i = 0; i < self->num_lanes; i++)
        {
          track_lane_update_track_name_hash (self->lanes[i]);
        }
      automation_tracklist_update_track_name_hash (
        &self->automation_tracklist, self);

      for (int i = 0; i < self->num_markers; i++)
        {
          marker_set_track_name_hash (
            self->markers[i], new_hash);
        }

      for (int i = 0; i < self->num_chord_regions; i++)
        {
          ZRegion * r = self->chord_regions[i];
          r->id.track_name_hash = new_hash;
          region_update_identifier (r);
        }

      self->processor->track = self;

      /* update sends */
      if (self->channel)
        channel_update_track_name_hash (
          self->channel, old_hash, new_hash);

      GPtrArray * ports = g_ptr_array_new ();
      track_append_ports (self, ports, true);
      for (size_t i = 0; i < ports->len; i++)
        {
          Port * port = g_ptr_array_index (ports, i);
          g_return_if_fail (IS_PORT_AND_NONNULL (port));
          port_update_track_name_hash (port, self, new_hash);

          if (port_is_exposed_to_backend (port))
            {
              port_rename_backend (port);
            }
        }
      object_free_w_func_and_null (g_ptr_array_unref, ports);
    }

  if (track_is_in_active_project (self))
    {
      /* update children */
      track_update_children (self);

      /* update mixer selections */
      if (
        MIXER_SELECTIONS->has_any
        && MIXER_SELECTIONS->track_name_hash == old_hash)
        {
          MIXER_SELECTIONS->track_name_hash = new_hash;
        }

      /* change the clip editor region */
      if (
        CLIP_EDITOR->has_region
        && CLIP_EDITOR->region_id.track_name_hash == old_hash)
        {
          g_message (
            "updating clip editor region track to "
            "%s",
            self->name);
          CLIP_EDITOR->region_id.track_name_hash = new_hash;
        }
    }

  if (pub_events)
    {
      EVENTS_PUSH (ET_TRACK_NAME_CHANGED, self);
    }
}

/**
 * Fills in the given array (if non-NULL) with all
 * plugins in the track and returns the number of
 * plugins.
 */
int
track_get_plugins (const Track * const self, GPtrArray * arr)
{
  int num_pls = 0;
  if (track_type_has_channel (self->type))
    {
      Plugin * pls[100];
      num_pls += channel_get_plugins (self->channel, pls);

      if (arr)
        {
          for (int i = 0; i < num_pls; i++)
            {
              g_ptr_array_add (arr, pls[i]);
            }
        }
    }

  if (self->type == TRACK_TYPE_MODULATOR)
    {
      for (int i = 0; i < self->num_modulators; i++)
        {
          Plugin * pl = self->modulators[i];
          if (pl)
            {
              num_pls++;
              if (arr)
                {
                  g_ptr_array_add (arr, pl);
                }
            }
        }
    }

  return num_pls;
}

void
track_activate_all_plugins (Track * track, bool activate)
{
  GPtrArray * pls = g_ptr_array_new ();
  int         num_pls = track_get_plugins (track, pls);

  for (int i = 0; i < num_pls; i++)
    {
      Plugin * pl = (Plugin *) g_ptr_array_index (pls, i);

      if (!pl->instantiated && !pl->instantiation_failed)
        {
          GError * err = NULL;
          int      ret = plugin_instantiate (pl, NULL, &err);
          if (ret != 0)
            {
              HANDLE_ERROR (
                err, "%s", _ ("Failed to instantiate plugin"));
            }
        }

      if (pl->instantiated)
        {
          plugin_activate (pl, activate);
        }
    }

  g_ptr_array_unref (pls);
}

/**
 * Comment setter.
 *
 * @note This creates an undoable action.
 */
void
track_comment_setter (void * track, const char * comment)
{
  Track * self = (Track *) track;
  g_return_if_fail (IS_TRACK (self));

  track_set_comment (self, comment, F_UNDOABLE);
}

/**
 * @param undoable Create an undable action.
 */
void
track_set_comment (
  Track *      self,
  const char * comment,
  bool         undoable)
{
  if (undoable)
    {
      track_select (
        self, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_comment (
          TRACKLIST_SELECTIONS, comment, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track comment"));
          return;
        }
    }
  else
    {
      g_free_and_null (self->comment);
      self->comment = g_strdup (comment);
    }
}

/**
 * Comment getter.
 */
const char *
track_get_comment (void * track)
{
  Track * self = (Track *) track;
  g_return_val_if_fail (IS_TRACK (self), NULL);

  return self->comment;
}

/**
 * Sets the track color.
 */
void
track_set_color (
  Track *         self,
  const GdkRGBA * color,
  bool            undoable,
  bool            fire_events)
{
  if (undoable)
    {
      track_select (
        self, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_color (
          TRACKLIST_SELECTIONS, color, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track comment"));
          return;
        }
    }
  else
    {
      self->color = *color;

      if (fire_events)
        {
          EVENTS_PUSH (ET_TRACK_COLOR_CHANGED, self);
        }
    }
}

/**
 * Sets the track icon.
 */
void
track_set_icon (
  Track *      self,
  const char * icon_name,
  bool         undoable,
  bool         fire_events)
{
  if (undoable)
    {
      track_select (
        self, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

      GError * err = NULL;
      bool ret = tracklist_selections_action_perform_edit_icon (
        TRACKLIST_SELECTIONS, icon_name, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track icon"));
          return;
        }
    }
  else
    {
      self->icon_name = g_strdup (icon_name);

      if (fire_events)
        {
          /* TODO */
        }
    }
}

TrackType
track_type_get_from_string (const char * str)
{
  for (int i = 0; i <= TRACK_TYPE_MIDI_GROUP; i++)
    {
      if (string_is_equal (track_type_strings[i].str, str))
        {
          return (TrackType) i;
        }
    }
  g_return_val_if_reached (-1);
}

/**
 * Returns the plugin at the given slot, if any.
 *
 * @param slot The slot (ignored if instrument is
 *   selected.
 */
Plugin *
track_get_plugin_at_slot (
  Track *        self,
  PluginSlotType slot_type,
  int            slot)
{
  switch (slot_type)
    {
    case PLUGIN_SLOT_MIDI_FX:
      return self->channel->midi_fx[slot];
    case PLUGIN_SLOT_INSTRUMENT:
      return self->channel->instrument;
    case PLUGIN_SLOT_INSERT:
      return self->channel->inserts[slot];
    case PLUGIN_SLOT_MODULATOR:
      if (self->modulators && slot < self->num_modulators)
        {
          return self->modulators[slot];
        }
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  return NULL;
}

/**
 * Marks the track for bouncing.
 *
 * @param mark_children Whether to mark all
 *   children tracks as well. Used when exporting
 *   stems on the specific track stem only.
 * @param mark_parents Whether to mark all parent
 *   tracks as well.
 */
void
track_mark_for_bounce (
  Track * self,
  bool    bounce,
  bool    mark_regions,
  bool    mark_children,
  bool    mark_parents)
{
  if (!track_type_has_channel (self->type))
    {
      return;
    }

  g_debug (
    "marking %s for bounce %d, mark regions %d", self->name,
    bounce, mark_regions);

  self->bounce = bounce;

  /* bounce directly to master if not marking
   * parents*/
  self->bounce_to_master = !mark_parents;

  if (mark_regions)
    {
      for (int j = 0; j < self->num_lanes; j++)
        {
          TrackLane * lane = self->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              ZRegion * r = lane->regions[k];
              if (
                r->id.type != REGION_TYPE_MIDI
                && r->id.type != REGION_TYPE_AUDIO)
                continue;

              r->bounce = bounce;
            }
        }

      for (int i = 0; i < self->num_chord_regions; i++)
        {
          ZRegion * r = self->chord_regions[i];
          r->bounce = bounce;
        }
    }

  /* also mark all parents */
  g_return_if_fail (track_type_has_channel (self->type));
  Track * direct_out =
    channel_get_output_track (self->channel);
  if (direct_out && mark_parents)
    {
      track_mark_for_bounce (
        direct_out, bounce, F_NO_MARK_REGIONS,
        F_NO_MARK_CHILDREN, F_MARK_PARENTS);
    }

  if (mark_children)
    {
      for (int i = 0; i < self->num_children; i++)
        {
          Track * child = tracklist_find_track_by_name_hash (
            TRACKLIST, self->children[i]);

          track_mark_for_bounce (
            child, bounce, mark_regions, F_MARK_CHILDREN,
            F_NO_MARK_PARENTS);
        }
    }
}

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 */
void
track_append_ports (
  Track *     self,
  GPtrArray * ports,
  bool        include_plugins)
{
  if (track_type_has_channel (self->type))
    {
      g_return_if_fail (self->channel);
      channel_append_ports (
        self->channel, ports, include_plugins);
      track_processor_append_ports (self->processor, ports);
    }

#define _ADD(port) \
  if (port) \
    { \
      g_ptr_array_add (ports, port); \
    } \
  else \
    { \
      g_warn_if_fail (port); \
    }

  if (track_type_can_record (self->type))
    {
      _ADD (self->recording);
    }

  if (self->type == TRACK_TYPE_TEMPO)
    {
      /* add bpm/time sig ports */
      _ADD (self->bpm_port);
      _ADD (self->beats_per_bar_port);
      _ADD (self->beat_unit_port);
    }
  else if (self->type == TRACK_TYPE_MODULATOR)
    {
      for (int j = 0; j < self->num_modulators; j++)
        {
          Plugin * pl = self->modulators[j];

          plugin_append_ports (pl, ports);
        }
      for (int j = 0; j < self->num_modulator_macros; j++)
        {
          _ADD (self->modulator_macros[j]->macro);
          _ADD (self->modulator_macros[j]->cv_in);
          _ADD (self->modulator_macros[j]->cv_out);
        }
    }

#undef _ADD
}

bool
track_is_enabled (Track * self)
{
  return self->enabled;
}

void
track_set_enabled (
  Track * self,
  bool    enabled,
  bool    trigger_undo,
  bool    auto_select,
  bool    fire_events)
{
  self->enabled = enabled;

  g_message (
    "Setting track %s enabled (%d)", self->name, enabled);
  if (auto_select)
    {
      track_select (self, F_SELECT, F_EXCLUSIVE, fire_events);
    }

  if (trigger_undo)
    {
      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_enable (
          TRACKLIST_SELECTIONS, enabled, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track enabled"));
          return;
        }
    }
  else
    {
      self->enabled = enabled;

      if (fire_events)
        {
          EVENTS_PUSH (ET_TRACK_STATE_CHANGED, self);
        }
    }
}

static void
get_end_pos_from_objs (
  ArrangerObject ** objs,
  int               num_objs,
  Position *        pos)
{
  for (int i = 0; i < num_objs; i++)
    {
      ArrangerObject * obj = objs[i];
      Position         end_pos;
      if (arranger_object_type_has_length (obj->type))
        {
          arranger_object_get_end_pos (obj, &end_pos);
        }
      else
        {
          arranger_object_get_pos (obj, &end_pos);
        }
      if (position_is_after (&end_pos, pos))
        {
          position_set_to_pos (pos, &end_pos);
        }
    }
}

void
track_get_total_bars (Track * self, int * total_bars)
{
  Position pos;
  position_from_bars (&pos, *total_bars);

  for (int i = 0; i < self->num_lanes; i++)
    {
      TrackLane * lane = self->lanes[i];
      get_end_pos_from_objs (
        (ArrangerObject **) lane->regions, lane->num_regions,
        &pos);
    }

  get_end_pos_from_objs (
    (ArrangerObject **) self->chord_regions,
    self->num_chord_regions, &pos);
  get_end_pos_from_objs (
    (ArrangerObject **) self->scales, self->num_scales, &pos);
  get_end_pos_from_objs (
    (ArrangerObject **) self->markers, self->num_markers,
    &pos);

  int track_total_bars = position_get_total_bars (&pos, true);
  if (track_total_bars > *total_bars)
    {
      *total_bars = track_total_bars;
    }
}

Track *
track_create_with_action (
  TrackType             type,
  const PluginSetting * pl_setting,
  const SupportedFile * file_descr,
  Position *            pos,
  int                   index,
  int                   num_tracks,
  GError **             error)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_create (
    type, pl_setting, file_descr, index, pos, num_tracks, -1,
    &err);
  if (!ret)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", _ ("Failed to create track"));
      return NULL;
    }

  Track * track = tracklist_get_track (TRACKLIST, index);
  g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), NULL);
  z_return_val_if_fail_cmp (track->type, ==, type, NULL);
  z_return_val_if_fail_cmp (track->pos, ==, index, NULL);
  return track;
}

/**
 * Removes the AutomationTrack's associated with
 * this channel from the AutomationTracklist in the
 * corresponding Track.
 */
static void
remove_ats_from_automation_tracklist (
  Track * track,
  bool    fire_events)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  g_return_if_fail (atl);

  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];
      if (
        at->port_id.flags & PORT_FLAG_CHANNEL_FADER
        || at->port_id.flags & PORT_FLAG_FADER_MUTE
        || at->port_id.flags & PORT_FLAG_STEREO_BALANCE)
        {
          automation_tracklist_remove_at (
            atl, at, F_NO_FREE, fire_events);
        }
    }
}

/**
 * Set various caches (snapshots, track name hash, plugin
 * input/output ports, etc).
 */
void
track_set_caches (Track * self, CacheTypes types)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (self);

  if (types & CACHE_TYPE_TRACK_NAME_HASHES)
    {
      /* update name hash */
      self->name_hash = track_get_name_hash (self);
    }

  if (
    types & CACHE_TYPE_PLAYBACK_SNAPSHOTS
    && !track_is_auditioner (self))
    {
      g_return_if_fail (
        g_atomic_int_get (&AUDIO_ENGINE->run) == 0);

      /* normal track lanes */
      for (int i = 0; i < self->num_lane_snapshots; i++)
        {
          track_lane_free (self->lane_snapshots[i]);
        }
      self->num_lane_snapshots = 0;
      self->lane_snapshots = g_realloc_n (
        self->lane_snapshots, (size_t) self->num_lanes,
        sizeof (TrackLane *));
      for (int i = 0; i < self->num_lanes; i++)
        {
          self->lane_snapshots[i] =
            track_lane_gen_snapshot (self->lanes[i]);
          self->num_lane_snapshots++;
        }

      /* chord regions */
      for (int i = 0; i < self->num_chord_region_snapshots; i++)
        {
          arranger_object_free (
            (ArrangerObject *) self->chord_region_snapshots[i]);
        }
      self->num_chord_region_snapshots = 0;
      self->chord_region_snapshots = g_realloc_n (
        self->chord_region_snapshots,
        (size_t) self->num_chord_regions, sizeof (ZRegion *));
      for (int i = 0; i < self->num_chord_regions; i++)
        {
          self->chord_region_snapshots[i] =
            (ZRegion *) arranger_object_clone (
              (ArrangerObject *) self->chord_regions[i]);
          self->num_chord_region_snapshots++;
        }

      /* scales */
      for (int i = 0; i < self->num_scale_snapshots; i++)
        {
          arranger_object_free (
            (ArrangerObject *) self->scale_snapshots[i]);
        }
      self->num_scale_snapshots = 0;
      self->scale_snapshots = g_realloc_n (
        self->scale_snapshots, (size_t) self->num_scales,
        sizeof (ScaleObject *));
      for (int i = 0; i < self->num_scales; i++)
        {
          self->scale_snapshots[i] =
            (ScaleObject *) arranger_object_clone (
              (ArrangerObject *) self->scales[i]);
          self->num_scale_snapshots++;
        }

      /* markers */
      for (int i = 0; i < self->num_marker_snapshots; i++)
        {
          arranger_object_free (
            (ArrangerObject *) self->marker_snapshots[i]);
        }
      self->num_marker_snapshots = 0;
      self->marker_snapshots = g_realloc_n (
        self->marker_snapshots, (size_t) self->num_markers,
        sizeof (Marker *));
      for (int i = 0; i < self->num_markers; i++)
        {
          self->marker_snapshots[i] =
            (Marker *) arranger_object_clone (
              (ArrangerObject *) self->markers[i]);
          self->num_marker_snapshots++;
        }

      /* automation regions */
      if (atl)
        {
          automation_tracklist_set_caches (
            atl, CACHE_TYPE_PLAYBACK_SNAPSHOTS);
        }
    }

  if (types & CACHE_TYPE_PLUGIN_PORTS)
    {
      if (track_type_has_channel (self->type))
        {
          channel_set_caches (self->channel);
        }
    }

  if (
    types & CACHE_TYPE_AUTOMATION_LANE_RECORD_MODES
    || types & CACHE_TYPE_AUTOMATION_LANE_PORTS)
    {
      if (atl)
        {
          automation_tracklist_set_caches (
            atl,
            CACHE_TYPE_AUTOMATION_LANE_RECORD_MODES
              | CACHE_TYPE_AUTOMATION_LANE_PORTS);
        }
    }
}

/**
 * Wrapper for each track type.
 */
void
track_free (Track * self)
{
  g_debug (
    "freeing track '%s' (pos %d)...", self->name, self->pos);

  if (Z_IS_TRACK_WIDGET (self->widget))
    {
      self->widget->track = NULL;
    }

  /* remove lanes */
  for (int i = 0; i < self->num_lanes; i++)
    {
      object_free_w_func_and_null (
        track_lane_free, self->lanes[i]);
    }
  object_zero_and_free (self->lanes);
  for (int i = 0; i < self->num_lane_snapshots; i++)
    {
      object_free_w_func_and_null (
        track_lane_free, self->lane_snapshots[i]);
    }
  object_zero_and_free (self->lane_snapshots);

  /* remove automation points, curves, tracks,
   * lanes*/
  automation_tracklist_free_members (
    &self->automation_tracklist);

  /* remove chords */
  for (int i = 0; i < self->num_chord_regions; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *,
        self->chord_regions[i]);
    }
  object_zero_and_free (self->chord_regions);
  for (int i = 0; i < self->num_chord_region_snapshots; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *,
        self->chord_region_snapshots[i]);
    }
  object_zero_and_free (self->chord_region_snapshots);

  /* remove scales */
  for (int i = 0; i < self->num_scales; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *,
        self->scales[i]);
    }
  object_zero_and_free (self->scales);
  for (int i = 0; i < self->num_scale_snapshots; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *,
        self->scale_snapshots[i]);
    }
  object_zero_and_free (self->scale_snapshots);

  /* remove markers */
  for (int i = 0; i < self->num_markers; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *,
        self->markers[i]);
    }
  object_zero_and_free (self->markers);
  for (int i = 0; i < self->num_marker_snapshots; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *,
        self->marker_snapshots[i]);
    }
  object_zero_and_free (self->marker_snapshots);

  if (self->bpm_port)
    {
      port_disconnect_all (self->bpm_port);
      object_free_w_func_and_null (port_free, self->bpm_port);
    }
  if (self->beats_per_bar_port)
    {
      port_disconnect_all (self->beats_per_bar_port);
      object_free_w_func_and_null (
        port_free, self->beats_per_bar_port);
    }
  if (self->beats_per_bar_port)
    {
      port_disconnect_all (self->beat_unit_port);
      object_free_w_func_and_null (
        port_free, self->beat_unit_port);
    }

#undef _FREE_TRACK

  if (self->channel)
    {
      /* remove automation tracks - they are
       * already free'd by now */
      remove_ats_from_automation_tracklist (
        self, F_NO_PUBLISH_EVENTS);
    }

  object_free_w_func_and_null (
    track_processor_free, self->processor);
  object_free_w_func_and_null (channel_free, self->channel);

  g_free_and_null (self->name);
  g_free_and_null (self->comment);
  g_free_and_null (self->icon_name);

  for (int i = 0; i < self->num_modulator_macros; i++)
    {
      modulator_macro_processor_free (
        self->modulator_macros[i]);
    }

  object_zero_and_free (self);

  g_debug ("done freeing track");
}
