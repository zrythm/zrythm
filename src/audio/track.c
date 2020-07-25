/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "actions/edit_tracks_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_group_track.h"
#include "audio/audio_region.h"
#include "audio/audio_track.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/control_port.h"
#include "audio/group_target_track.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_bus_track.h"
#include "audio/midi_group_track.h"
#include "audio/midi_track.h"
#include "audio/instrument_track.h"
#include "audio/router.h"
#include "audio/stretcher.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <glib/gi18n.h>

void
track_init_loaded (
  Track * self,
  bool    project)
{
  self->magic = TRACK_MAGIC;

  if (TRACK_CAN_BE_GROUP_TARGET (self))
    {
      group_target_track_init_loaded (self);
    }

  TrackLane * lane;
  for (int j = 0; j < self->num_lanes; j++)
    {
      lane = self->lanes[j];
      track_lane_init_loaded (lane);
    }
  ScaleObject * scale;
  for (int i = 0; i < self->num_scales; i++)
    {
      scale = self->scales[i];
      arranger_object_init_loaded (
        (ArrangerObject *) scale);
    }
  Marker * marker;
  for (int i = 0; i < self->num_markers; i++)
    {
      marker = self->markers[i];
      arranger_object_init_loaded (
        (ArrangerObject *) marker);
    }
  ZRegion * region;
  for (int i = 0; i < self->num_chord_regions; i++)
    {
      region = self->chord_regions[i];
      region->id.track_pos = self->pos;
      arranger_object_init_loaded (
        (ArrangerObject *) region);
    }

  /* init loaded channel */
  if (self->channel)
    {
      self->processor->track = self;
      track_processor_init_loaded (
        self->processor, project);

      self->channel->track = self;
      channel_init_loaded (
        self->channel, project);
    }

  /* set track to automation tracklist */
  AutomationTracklist * atl =
    track_get_automation_tracklist (self);
  if (atl)
    {
      automation_tracklist_init_loaded (atl);
    }

  if (self->type == TRACK_TYPE_AUDIO)
    {
      self->rt_stretcher =
        stretcher_new_rubberband (
          AUDIO_ENGINE->sample_rate, 2, 1.0,
          1.0, true);
    }

  /** set magic to all track ports */
  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  Port * port;
  track_append_all_ports (
    self, &ports, &num_ports, true, &max_size,
    true);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];
      port->magic = PORT_MAGIC;
    }
  free (ports);

  track_set_is_project (self, project);
}

/**
 * Adds a new TrackLane to the Track.
 */
static void
track_add_lane (
  Track * self,
  int     fire_events)
{
  g_return_if_fail (self);
  array_double_size_if_full (
    self->lanes, self->num_lanes,
    self->lanes_size, TrackLane *);
  g_message ("-----num lanes %d, lanes size %d",
    self->num_lanes, self->lanes_size);
  self->lanes[self->num_lanes++] =
    track_lane_new (self, self->num_lanes);

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_TRACK_LANE_ADDED,
        self->lanes[self->num_lanes - 1]);
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
track_init (
  Track *   self,
  const int add_lane)
{
  self->visible = 1;
  self->main_height = TRACK_DEF_HEIGHT;
  self->midi_ch = 1;
  self->magic = TRACK_MAGIC;
  self->comment = g_strdup ("");
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
  TrackType type,
  int       pos,
  char *    label,
  const int with_lane)
{
  Track * self = object_new (Track);

  self->pos = pos;
  self->type = type;
  track_init (self, with_lane);

  self->name = g_strdup (label);

  switch (type)
    {
    case TRACK_TYPE_INSTRUMENT:
      self->in_signal_type =
        TYPE_EVENT;
      self->out_signal_type =
        TYPE_AUDIO;
      instrument_track_init (self);
      break;
    case TRACK_TYPE_AUDIO:
      self->in_signal_type =
        TYPE_AUDIO;
      self->out_signal_type =
        TYPE_AUDIO;
      audio_track_init (self);
      break;
    case TRACK_TYPE_MASTER:
      self->in_signal_type =
        TYPE_AUDIO;
      self->out_signal_type =
        TYPE_AUDIO;
      master_track_init (self);
      break;
    case TRACK_TYPE_AUDIO_BUS:
      self->in_signal_type =
        TYPE_AUDIO;
      self->out_signal_type =
        TYPE_AUDIO;
      audio_bus_track_init (self);
      break;
    case TRACK_TYPE_MIDI_BUS:
      self->in_signal_type =
        TYPE_EVENT;
      self->out_signal_type =
        TYPE_EVENT;
      midi_bus_track_init (self);
      break;
    case TRACK_TYPE_AUDIO_GROUP:
      self->in_signal_type =
        TYPE_AUDIO;
      self->out_signal_type =
        TYPE_AUDIO;
      audio_group_track_init (self);
      break;
    case TRACK_TYPE_MIDI_GROUP:
      self->in_signal_type =
        TYPE_EVENT;
      self->out_signal_type =
        TYPE_EVENT;
      midi_group_track_init (self);
      break;
    case TRACK_TYPE_MIDI:
      self->in_signal_type =
        TYPE_EVENT;
      self->out_signal_type =
        TYPE_EVENT;
      midi_track_init (self);
      break;
    case TRACK_TYPE_CHORD:
      self->in_signal_type =
        TYPE_EVENT;
      self->out_signal_type =
        TYPE_EVENT;
      chord_track_init (self);
      break;
    case TRACK_TYPE_MARKER:
      self->in_signal_type =
        TYPE_UNKNOWN;
      self->out_signal_type =
        TYPE_UNKNOWN;
      marker_track_init (self);
      break;
    case TRACK_TYPE_TEMPO:
      self->in_signal_type =
        TYPE_UNKNOWN;
      self->out_signal_type =
        TYPE_UNKNOWN;
      tempo_track_init (self);
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  if (TRACK_CAN_BE_GROUP_TARGET (self))
    {
      group_target_track_init (self);
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
 * Clones the track and returns the clone.
 *
 * @bool src_is_project Whether \ref track is a
 *   project track.
 */
Track *
track_clone (
  Track * track,
  bool    src_is_project)
{
  int j;
  Track * new_track =
    track_new (
      track->type, track->pos, track->name,
      F_WITHOUT_LANE);

#define COPY_MEMBER(a) \
  new_track->a = track->a

  COPY_MEMBER (type);
  COPY_MEMBER (automation_visible);
  COPY_MEMBER (visible);
  COPY_MEMBER (main_height);
  COPY_MEMBER (recording);
  COPY_MEMBER (pinned);
  COPY_MEMBER (active);
  COPY_MEMBER (color);
  COPY_MEMBER (pos);
  COPY_MEMBER (midi_ch);

#undef COPY_MEMBER

  if (track->channel)
    {
      Channel * ch =
        channel_clone (
          track->channel, new_track,
          src_is_project);
      new_track->channel = ch;
    }

  TrackLane * lane, * new_lane;
  new_track->num_lanes = track->num_lanes;
  new_track->lanes =
    realloc (
      new_track->lanes,
      sizeof (TrackLane *) *
        (size_t) track->num_lanes);
  for (j = 0; j < track->num_lanes; j++)
    {
      /* clone lane */
       lane = track->lanes[j];
       new_lane =
         track_lane_clone (lane);
       new_lane->track_pos = new_track->pos;
       new_track->lanes[j] = new_lane;
    }

  automation_tracklist_clone (
    &track->automation_tracklist,
    &new_track->automation_tracklist);

  if (TRACK_CAN_BE_GROUP_TARGET (track))
    {
      for (int i = 0; i < track->num_children; i++)
        {
          group_target_track_add_child (
            new_track, track->children[i],
            false, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);
        }
    }

  /* check that source track is not affected
   * during unit tests */
  if (ZRYTHM_TESTING && src_is_project)
    {
      track_verify_identifiers (track);
    }

  return new_track;
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
  int     select,
  int     exclusive,
  int     fire_events)
{
  g_return_if_fail (IS_TRACK (self));

  if (select)
    {
      if (exclusive)
        {
          tracklist_selections_select_single (
            TRACKLIST_SELECTIONS, self);
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
      EVENTS_PUSH (
        ET_TRACK_CHANGED, self);
    }
}

/**
 * Returns if the track is soloed.
 */
bool
track_get_soloed (
  Track * self)
{
  g_return_val_if_fail (
    self && self->channel, false);
  return fader_get_soloed (self->channel->fader);
}

/**
 * Returns if the track is muted.
 */
bool
track_get_muted (
  Track * self)
{
  g_return_val_if_fail (
    self && self->channel, false);
  return fader_get_muted (self->channel->fader);
}

TrackType
track_get_type_from_plugin_descriptor (
  PluginDescriptor * descr)
{
  if (plugin_descriptor_is_instrument (descr))
    return TRACK_TYPE_INSTRUMENT;
  else if (plugin_descriptor_is_midi_modifier (
             descr))
    return TRACK_TYPE_MIDI;
  else
    return TRACK_TYPE_AUDIO_BUS;
}

/**
 * Sets recording and connects/disconnects the
 * JACK ports.
 */
void
track_set_recording (
  Track *   track,
  bool      recording,
  bool      fire_events)
{
  Channel * channel =
    track_get_channel (track);

  if (!channel)
    {
      g_message (
        "Recording not implemented yet for this "
        "track.");
      return;
    }

  /*if (recording)*/
    /*{*/
      /* FIXME just enable the connections */
      /*port_connect (*/
        /*AUDIO_ENGINE->stereo_in->l,*/
        /*channel->stereo_in->l, 1);*/
      /*port_connect (*/
        /*AUDIO_ENGINE->stereo_in->r,*/
        /*channel->stereo_in->r, 1);*/
      /*port_connect (*/
        /*AUDIO_ENGINE->midi_in,*/
        /*channel->midi_in, 1);*/
    /*}*/
  /*else*/
    /*{*/
      /* FIXME just disable the connections */
      /*port_disconnect (*/
        /*AUDIO_ENGINE->stereo_in->l,*/
        /*channel->stereo_in->l);*/
      /*port_disconnect (*/
        /*AUDIO_ENGINE->stereo_in->r,*/
        /*channel->stereo_in->r);*/
      /*port_disconnect (*/
        /*AUDIO_ENGINE->midi_in,*/
        /*channel->midi_in);*/
    /*}*/

  track->recording = recording;

  if (fire_events)
    {
      EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
                   track);
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
  bool    fire_events)
{
  g_return_if_fail (self && self->channel);

  g_message (
    "Setting track %s muted (%d)",
    self->name, mute);
  fader_set_muted (
    self->channel->fader, mute, trigger_undo,
    fire_events);
}

/**
 * Returns the Track from the Project matching
 * \p name.
 *
 * @param name Name to search for.
 */
Track *
track_get_from_name (
  const char * name)
{
  g_return_val_if_fail (name, NULL);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (string_is_equal (track->name, name, 1))
        {
          return track;
        }
    }

  g_return_val_if_reached (NULL);
}

Track *
track_find_by_name (
  const char * name)
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
  int *            velocities_size,
  int              inside)
{
  if (track->type != TRACK_TYPE_MIDI &&
      track->type != TRACK_TYPE_INSTRUMENT)
    return;

  TrackLane * lane;
  ZRegion * region;
  MidiNote * mn;
  Position global_start_pos;
  for (int i = 0; i < track->num_lanes; i++)
    {
      lane = track->lanes[i];
      for (int j = 0; j < lane->num_regions; j++)
        {
          region = lane->regions[j];
          for (int k = 0;
               k < region->num_midi_notes; k++)
            {
              mn = region->midi_notes[k];
              midi_note_get_global_start_pos (
                mn, &global_start_pos);

#define ADD_VELOCITY \
  array_double_size_if_full ( \
    *velocities, *num_velocities, \
    *velocities_size, Velocity *); \
  (*velocities)[(* num_velocities)++] = \
    mn->vel

              if (inside &&
                  position_is_after_or_equal (
                    &global_start_pos, start_pos) &&
                  position_is_before_or_equal (
                    &global_start_pos, end_pos))
                {
                  ADD_VELOCITY;
                }
              else if (!inside &&
                  (position_is_before (
                    &global_start_pos, start_pos) ||
                  position_is_after (
                    &global_start_pos, end_pos)))
                {
                  ADD_VELOCITY;
                }
#undef ADD_VELOCITY
            }
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
track_verify_identifiers (
  Track * self)
{
  g_return_val_if_fail (self, false);

  g_message (
    "verifying %s identifiers...", self->name);

  int track_pos = self->pos;

  /* verify port identifiers */
  int max_size = 20;
  int num_ports = 0;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  track_append_all_ports (
    self, &ports, &num_ports, true, &max_size,
    true);
  AutomationTracklist * atl =
    track_get_automation_tracklist (self);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      g_return_val_if_fail (
        port->id.track_pos == track_pos, false);
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          PluginIdentifier * pid =
            &port->id.plugin_id;
          g_return_val_if_fail (
            pid->track_pos == track_pos, false);
          Plugin * pl = plugin_find (pid);
          g_return_val_if_fail (
            plugin_identifier_is_equal (
              &pl->id, pid), false);
          if (pid->slot_type ==
                PLUGIN_SLOT_INSTRUMENT)
            {
              g_return_val_if_fail (
                pl == self->channel->instrument,
                false);
            }
        }

      /* check that the automation track is there */
      if (atl &&
          port->id.flags & PORT_FLAG_AUTOMATABLE)
        {
          /*g_message ("checking %s", port->id.label);*/
          AutomationTrack * at =
            automation_track_find_from_port (
              port, self, true);
          g_return_val_if_fail (at, false);
          g_return_val_if_fail (
            automation_track_find_from_port (
              port, self, false), false);
        }

      port_verify_src_and_dests (port);
    }
  free (ports);

  /* verify tracklist identifiers */
  if (atl)
    {
      g_return_val_if_fail (
        automation_tracklist_verify_identifiers (
          atl),
        false);
    }

  g_message ("done");

  return true;
}

/**
 * Returns if the given TrackType can host the
 * given RegionType.
 */
int
track_type_can_host_region_type (
  const TrackType  tt,
  const RegionType rt)
{
  switch (rt)
    {
    case REGION_TYPE_MIDI:
      return
        tt == TRACK_TYPE_MIDI ||
        tt == TRACK_TYPE_INSTRUMENT;
    case REGION_TYPE_AUDIO:
      return
        tt == TRACK_TYPE_AUDIO;
    case REGION_TYPE_AUTOMATION:
      return
        tt != TRACK_TYPE_CHORD &&
        tt != TRACK_TYPE_MARKER;
    case REGION_TYPE_CHORD:
      return
        tt == TRACK_TYPE_CHORD;
    }
  g_return_val_if_reached (-1);
}

/**
 * Returns the full visible height (main height +
 * height of all visible automation tracks + height
 * of all visible lanes).
 */
int
track_get_full_visible_height (
  Track * self)
{
  int height = self->main_height;

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

/**
 * Sets track soloed, updates UI and optionally
 * adds the action to the undo stack.
 */
void
track_set_soloed (
  Track * self,
  bool    solo,
  bool    trigger_undo,
  bool    fire_events)
{
  g_return_if_fail (self && self->channel);
  fader_set_soloed (
    self->channel->fader, solo, trigger_undo,
    fire_events);
}

/**
 * Writes the track to the given MIDI file.
 */
void
track_write_to_midi_file (
  Track *     self,
  MIDI_FILE * mf)
{
  g_return_if_fail (
    track_has_piano_roll (self));

  TrackLane * lane;
  for (int i = 0; i < self->num_lanes; i++)
    {
      lane = self->lanes[i];

      track_lane_write_to_midi_file (
        lane, mf);
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

/**
 * Returns the last region in the track, or NULL.
 *
 * FIXME cache.
 */
ZRegion *
track_get_last_region (
  Track * track)
{
  int i, j;
  ZRegion * last_region = NULL, * r;
  ArrangerObject * r_obj;
  Position tmp;
  position_init (&tmp);

  if (track->type == TRACK_TYPE_AUDIO ||
      track->type == TRACK_TYPE_INSTRUMENT)
    {
      TrackLane * lane;
      for (i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              r_obj = (ArrangerObject *) r;
              if (position_is_after (
                    &r_obj->end_pos, &tmp))
                {
                  last_region = r;
                  position_set_to_pos (
                    &tmp, &r_obj->end_pos);
                }
            }
        }
    }

  AutomationTracklist * atl =
    &track->automation_tracklist;
  AutomationTrack * at;
  for (i = 0; i < atl->num_ats; i++)
    {
      at = atl->ats[i];
      r = automation_track_get_last_region (at);
      r_obj = (ArrangerObject *) r;

      if (!r)
        continue;

      if (position_is_after (
            &r_obj->end_pos, &tmp))
        {
          last_region = r;
          position_set_to_pos (
            &tmp, &r_obj->end_pos);
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
track_generate_automation_tracks (
  Track * track)
{
  g_message (
    "generating for %s", track->name);

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at;

  if (track_type_has_channel (track->type))
    {
      /* -- fader -- */

      /* volume */
      at =
        automation_track_new (
          track->channel->fader->amp);
      automation_tracklist_add_at (atl, at);
      at->created = 1;
      at->visible = 1;

      /* balance */
      at =
        automation_track_new (
          track->channel->fader->balance);
      automation_tracklist_add_at (atl, at);

      /* mute */
      at =
        automation_track_new (
          track->channel->fader->mute);
      automation_tracklist_add_at (atl, at);

      /*  -- prefader -- */

      /* volume */
      at =
        automation_track_new (
          track->channel->prefader->amp);
      automation_tracklist_add_at (atl, at);

      /* balance */
      at =
        automation_track_new (
          track->channel->prefader->balance);
      automation_tracklist_add_at (atl, at);

      /* mute */
      at =
        automation_track_new (
          track->channel->prefader->mute);
      automation_tracklist_add_at (atl, at);
    }

  if (track_has_piano_roll (track))
    {
      /* midi automatables */
      for (int i = 0;
           i < NUM_MIDI_AUTOMATABLES * 16; i++)
        {
          Port * cc =
            track->processor->midi_automatables[i];
          at = automation_track_new (cc);
          automation_tracklist_add_at (atl, at);
        }
    }

  /* create special BPM and time sig automation
   * tracks for tempo track */
  if (track->type == TRACK_TYPE_TEMPO)
    {
      at = automation_track_new (track->bpm_port);
      at->created = true;
      at->visible = true;
      automation_tracklist_add_at (atl, at);
      at =
        automation_track_new (
          track->time_sig_port);
      automation_tracklist_add_at (atl, at);
    }

  g_message ("done");
}

/**
 * Wrapper.
 */
void
track_setup (Track * track)
{
#define SETUP_TRACK(uc,sc) \
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
 * Adds a ZRegion to the given lane of the track.
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
  Track * track,
  ZRegion * region,
  AutomationTrack * at,
  int      lane_pos,
  int      gen_name,
  int      fire_events)
{
  if (region->id.type == REGION_TYPE_AUTOMATION)
    {
      track = automation_track_get_track (at);
    }
  g_warn_if_fail (track);

  if (gen_name)
    {
      region_gen_name (
        region, NULL, at, track);
    }

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

      g_warn_if_fail (track->lanes[lane_pos]);
      track_lane_add_region (
        track->lanes[lane_pos], region);
    }

  if (add_at)
    {
      automation_track_add_region (
        at, region);
    }

  if (add_chord)
    {
      g_warn_if_fail (track == P_CHORD_TRACK);
      array_double_size_if_full (
        track->chord_regions,
        track->num_chord_regions,
        track->chord_regions_size, ZRegion *);
      array_append (
        track->chord_regions,
        track->num_chord_regions, region);
      region->id.idx = track->num_chord_regions - 1;
    }

  /* write clip if audio region */
  if (region->id.type == REGION_TYPE_AUDIO)
    {
      AudioClip * clip =
        audio_region_get_clip (region);
      audio_clip_write_to_pool (clip);
    }

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_CREATED, region);
    }
}

/**
 * Creates missing TrackLane's until pos.
 */
void
track_create_missing_lanes (
  Track *   track,
  const int pos)
{
  while (track->num_lanes < pos + 2)
    {
      track_add_lane (track, 0);
    }
}

/**
 * Removes the empty last lanes of the Track
 * (except the last one).
 */
void
track_remove_empty_last_lanes (
  Track * track)
{
  g_return_if_fail (track);
  g_message ("removing empty last lanes from %s",
             track->name);
  int removed = 0;
  for (int i = track->num_lanes - 1; i >= 1; i--)
    {
      g_message ("lane %d has %d regions",
                 i, track->lanes[i]->num_regions);
      if (track->lanes[i]->num_regions > 0)
        break;

      if (track->lanes[i]->num_regions == 0 &&
          track->lanes[i - 1]->num_regions == 0)
        {
          g_message ("removing lane %d", i);
          track->num_lanes--;
          free_later (
            track->lanes[i], track_lane_free);
          removed = 1;
        }
    }

  if (removed)
    {
      EVENTS_PUSH (ET_TRACK_LANE_REMOVED, NULL);
    }
}

/**
 * Returns if the Track should have a piano roll.
 */
int
track_has_piano_roll (
  const Track * track)
{
  return track->type == TRACK_TYPE_MIDI ||
    track->type == TRACK_TYPE_INSTRUMENT;
}

/**
 * Updates position in the tracklist and also
 * updates the information in the lanes.
 */
void
track_set_pos (
  Track * self,
  int     pos)
{
  g_message (
    "%s (%d) to %d",
    self->name, self->pos, pos);
  /*int prev_pos = self->pos;*/
  self->pos = pos;

  for (int i = 0; i < self->num_lanes; i++)
    {
      track_lane_set_track_pos (
        self->lanes[i], pos);
    }
  automation_tracklist_update_track_pos (
    &self->automation_tracklist, self);

  track_processor_set_track_pos (
    self->processor, pos);
  self->processor->track = self;

  int max_size = 20;
  Port ** ports =
    calloc (
      (size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  track_append_all_ports (
    self, &ports, &num_ports, true,
    &max_size, true);
  for (int i = 0; i < num_ports; i++)
    {
      g_warn_if_fail (ports[i]);
      port_update_track_pos (ports[i], self, pos);
    }
  free (ports);

  /* update port identifier track positions */
  if (self->channel)
    {
      Channel * ch = self->channel;
      channel_update_track_pos (ch, pos);
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
track_disconnect (
  Track * self,
  bool    remove_pl,
  bool    recalc_graph)
{
  g_message ("disconnecting %s (%d)...",
    self->name, self->pos);

  /* disconnect all ports */
  int max_size = 20;
  Port ** ports =
    calloc (
      (size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  track_append_all_ports (
    self, &ports, &num_ports,
    true, &max_size, true);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      g_return_if_fail (
        IS_PORT (port) &&
        port->is_project == self->is_project);
      if (ZRYTHM_TESTING)
        {
          port_verify_src_and_dests (port);
        }
      port_disconnect_all (port);
    }
  free (ports);

  if (recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  if (track_type_has_channel (self->type))
    {
      channel_disconnect (
        self->channel, remove_pl);
    }

  g_message ("done");
}

/**
 * Set track lanes visible and fire events.
 */
void
track_set_lanes_visible (
  Track *   track,
  const int visible)
{
  track->lanes_visible = visible;

  EVENTS_PUSH (
    ET_TRACK_LANES_VISIBILITY_CHANGED, track);
}

/**
 * Set automation visible and fire events.
 */
void
track_set_automation_visible (
  Track *   track,
  const int visible)
{
  track->automation_visible = visible;

  EVENTS_PUSH (
    ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, track);
}

/**
 * Unselects all arranger objects in the track.
 */
void
track_unselect_all (
  Track * self)
{
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
track_clear (
  Track * self)
{
  /* remove lane regions */
  for (int i = 0; i < self->num_lanes; i++)
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
  Track *  track,
  ZRegion * region,
  int      fire_events,
  int      free)
{
  region_disconnect (region);

  g_warn_if_fail (region->id.lane_pos >= 0);

  if (region_type_has_lane (region->id.type))
    {
      TrackLane * lane =
        region_get_lane (region);
      track_lane_remove_region (lane, region);
    }
  else if (region->id.type == REGION_TYPE_CHORD)
    {
      array_delete (
        track->chord_regions,
        track->num_chord_regions,
        region);
    }
  else if (region->id.type ==
             REGION_TYPE_AUTOMATION)
    {
      AutomationTracklist * atl =
        &track->automation_tracklist;
      for (int i = 0; i < atl->num_ats; i++)
        {
          AutomationTrack * at = atl->ats[i];
          if (at->index == region->id.at_idx)
            {
              array_delete (
                at->regions, at->num_regions,
                region);
            }
        }
    }

  if (free)
    {
      arranger_object_free (
        (ArrangerObject *) region);
    }

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_REMOVED,
        ARRANGER_OBJECT_TYPE_REGION);
    }
}

/**
 * Adds and connects a Modulator to the Track.
 */
void
track_add_modulator (
  Track * track,
  Modulator * modulator)
{
  array_double_size_if_full (
    track->modulators, track->num_modulators,
    track->modulators_size, Modulator *);
  array_append (track->modulators,
                track->num_modulators,
                modulator);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  EVENTS_PUSH (ET_MODULATOR_ADDED, modulator);
}

/**
 * Returns the automation tracklist if the track type has one,
 * or NULL if it doesn't (like chord tracks).
 */
AutomationTracklist *
track_get_automation_tracklist (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_MARKER:
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
      return &track->automation_tracklist;
    default:
      g_warn_if_reached ();
      break;
    }

  return NULL;
}

/**
 * Returns the channel of the track, if the track type has
 * a channel,
 * or NULL if it doesn't.
 */
Channel *
track_get_channel (Track * track)
{
  g_warn_if_fail (track);

  switch (track->type)
    {
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_AUDIO_GROUP:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_MIDI_GROUP:
    case TRACK_TYPE_MIDI:
    case TRACK_TYPE_CHORD:
      return track->channel;
    default:
      return NULL;
    }
}

/**
 * Returns the region at the given position, or NULL.
 */
ZRegion *
track_get_region_at_pos (
  const Track *    track,
  const Position * pos)
{
  int i, j;

  if (track->type == TRACK_TYPE_INSTRUMENT ||
      track->type == TRACK_TYPE_AUDIO ||
      track->type == TRACK_TYPE_MIDI)
    {
      TrackLane * lane;
      ZRegion * r;
      ArrangerObject * r_obj;
      for (i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              r_obj = (ArrangerObject *) r;
              if (pos->frames >=
                    r_obj->pos.frames &&
                  pos->frames <=
                    r_obj->end_pos.frames)
                return r;
            }
        }
    }
  else if (track->type == TRACK_TYPE_CHORD)
    {
      ZRegion * r;
      ArrangerObject * r_obj;
      for (j = 0; j < track->num_chord_regions; j++)
        {
          r = track->chord_regions[j];
          r_obj = (ArrangerObject *) r;
          if (position_is_after_or_equal (
                pos, &r_obj->pos) &&
              position_is_before_or_equal (
                pos, &r_obj->end_pos))
            return r;
        }
    }

  return NULL;
}

const char *
track_stringize_type (
  TrackType type)
{
  return _(track_type_strings[type].str);
}

/**
 * Returns the FaderType corresponding to the given
 * Track.
 */
FaderType
track_get_fader_type (
  const Track * track)
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
track_type_get_prefader_type (
  TrackType type)
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
 * Updates the frames of each position in each child
 * of the track recursively.
 */
void
track_update_frames (
  Track * self)
{
  int i;
  for (i = 0; i < self->num_lanes; i++)
    {
      track_lane_update_frames (self->lanes[i]);
    }
  for (i = 0; i < self->num_chord_regions; i++)
    {
      arranger_object_update_frames (
        (ArrangerObject *)
        self->chord_regions[i]);
    }
  for (i = 0; i < self->num_scales; i++)
    {
      arranger_object_update_frames (
        (ArrangerObject *)
        self->scales[i]);
    }
  for (i = 0; i < self->num_markers; i++)
    {
      arranger_object_update_frames (
        (ArrangerObject *)
        self->markers[i]);
    }

  automation_tracklist_update_frames (
    &self->automation_tracklist);
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
          "%d %s",
          track->pos, track->name);
    }
#endif
  return track->name;
}

/**
 * Setter to be used by the UI.
 */
void
track_set_name_with_events (
  Track *      track,
  const char * name)
{
  track_set_name (track, name, F_PUBLISH_EVENTS);
}

int
track_get_regions_in_range (
  Track *    track,
  Position * p1,
  Position * p2,
  ZRegion ** regions)
{
  int i, j;
  int count = 0;

  if (track->type == TRACK_TYPE_INSTRUMENT ||
      track->type == TRACK_TYPE_AUDIO ||
      track->type == TRACK_TYPE_MIDI)
    {
      TrackLane * lane;
      ZRegion * r;
      ArrangerObject * r_obj;
      for (i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              r_obj = (ArrangerObject *) r;
              /* start inside */
              if ((position_is_before_or_equal (
                     p1, &r_obj->pos) &&
                   position_is_after (
                     p2, &r_obj->pos)) ||
                  /* end inside */
                  (position_is_before_or_equal (
                     p1, &r_obj->end_pos) &&
                   position_is_after (
                     p2, &r_obj->end_pos)) ||
                  /* start before and end after */
                  (position_is_before_or_equal (
                     &r_obj->pos, p1) &&
                   position_is_after (
                     &r_obj->end_pos, p2)))
                {
                  regions[count++] = r;
                }
            }
        }
    }
  else if (track->type == TRACK_TYPE_CHORD)
    {
      ZRegion * r;
      ArrangerObject * r_obj;
      for (j = 0; j < track->num_chord_regions; j++)
        {
          r = track->chord_regions[j];
          r_obj = (ArrangerObject *) r;
          /* start inside */
          if ((position_is_before_or_equal (
                 p1, &r_obj->pos) &&
               position_is_after (
                 p2, &r_obj->pos)) ||
              /* end inside */
              (position_is_before_or_equal (
                 p1, &r_obj->end_pos) &&
               position_is_after (
                 p2, &r_obj->end_pos)) ||
              /* start before and end after */
              (position_is_before_or_equal (
                 &r_obj->pos, p1) &&
               position_is_after (
                 &r_obj->end_pos, p2)))
            {
              regions[count++] = r;
            }
        }
    }

  return count;
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
track_set_name (
  Track *      track,
  const char * _name,
  int          pub_events)
{
  char name[800];
  strcpy (name, _name);
  while (!tracklist_track_name_is_unique (
            TRACKLIST, name, track))
    {
      char name_without_num[780];
      int ending_num =
        string_get_int_after_last_space (
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
            name, "%s %d",
            name_without_num, ending_num + 1);
        }
    }

  if (track->name)
    g_free (track->name);
  track->name =
    g_strdup (name);

  if (track->channel)
    {
      /* update external ports */
      int max_size = 20;
      Port ** ports =
        calloc (
          (size_t) max_size, sizeof (Port *));
      int num_ports = 0;
      track_append_all_ports (
        track, &ports, &num_ports,
        true, &max_size, true);
      Port * port;
      for (int i = 0; i < num_ports; i++)
        {
          port = ports[i];
          g_return_if_fail (port);

          if (port_is_exposed_to_backend (
                port))
            {
              port_rename_backend (port);
            }
        }
      free (ports);
    }

  if (pub_events)
    {
      EVENTS_PUSH (
        ET_TRACK_NAME_CHANGED, track);
    }
}

void
track_activate_all_plugins (
  Track * track,
  bool    activate)
{
  if (!track_type_has_channel (track->type))
    return;

  Channel * ch = track_get_channel (track);
  g_return_if_fail (ch);

  for (int i = 0; i < STRIP_SIZE * 2 + 1; i++)
    {
      Plugin * pl = NULL;
      if (i < STRIP_SIZE)
        pl = track->channel->midi_fx[i];
      else if (i == STRIP_SIZE)
        pl = track->channel->instrument;
      else
        pl =
          track->channel->inserts[
            i - (STRIP_SIZE + 1)];

      if (pl)
        {
          plugin_activate (pl, activate);
        }
    }
}

/**
 * Comment setter.
 */
void
track_set_comment (
  void *  track,
  char *  comment)
{
  Track * self = (Track *) track;
  g_return_if_fail (IS_TRACK (self));

  g_free (self->comment);
  self->comment = g_strdup (comment);
}

/**
 * Comment getter.
 */
const char *
track_get_comment (
  void *  track)
{
  Track * self = (Track *) track;
  g_return_val_if_fail (IS_TRACK (self), NULL);

  return self->comment;
}

/**
 * Recursively marks the track and children as
 * project objects or not.
 */
void
track_set_is_project (
  Track * self,
  bool    is_project)
{
  g_message ("Setting %s to %d...",
    self->name, is_project);

  track_processor_set_is_project (
    self->processor, is_project);
  if (self->channel)
    {
      fader_set_is_project (
        self->channel->fader, is_project);
    }

  /** set all track ports to non project */
  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  Port * port;
  track_append_all_ports (
    self, &ports, &num_ports, true, &max_size,
    true);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];
      g_return_if_fail (IS_PORT (port));
      /*g_message (*/
        /*"%s: setting %s (%p) to %d",*/
        /*__func__, port->id.label, port, is_project);*/
      port_set_is_project (port, is_project);
    }
  free (ports);

  /* activates/deactivates all plugins */
  if (self->channel)
    {
      Plugin * plugins[60];
      int num_plugins =
        channel_get_plugins (
          self->channel, plugins);
      for (int i = 0; i < num_plugins; i++)
        {
          Plugin * pl = plugins[i];
          plugin_activate (pl, is_project);
        }
    }

  self->is_project = is_project;

  g_message ("done");
}

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
  int *     max_size,
  bool      include_plugins)
{
  if (track_type_has_channel (self->type))
    {
      g_return_if_fail (self->channel);
      channel_append_all_ports (
        self->channel, ports, size, is_dynamic,
        max_size, include_plugins);
      track_processor_append_ports (
        self->processor, ports, size, is_dynamic,
        max_size);
    }

#define _ADD(port) \
  if (is_dynamic) \
    { \
      array_double_size_if_full ( \
        *ports, (*size), (*max_size), Port *); \
    } \
  else if (*size == *max_size) \
    { \
      g_return_if_reached (); \
    } \
  g_warn_if_fail (port); \
  array_append ( \
    *ports, (*size), port)

  if (self->type == TRACK_TYPE_TEMPO)
    {
      /* add bpm/time sig ports */
      _ADD (self->bpm_port);
      _ADD (self->time_sig_port);
    }

#undef _ADD
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
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];
      if (at->port_id.flags &
            PORT_FLAG_CHANNEL_FADER ||
          at->port_id.flags &
            PORT_FLAG_CHANNEL_MUTE ||
          at->port_id.flags &
            PORT_FLAG_STEREO_BALANCE)
        {
          automation_tracklist_remove_at (
            atl, at, F_NO_FREE, fire_events);
        }
    }
}

/**
 * Wrapper for each track type.
 */
void
track_free (Track * self)
{
  g_message ("freeing %s (%d)...",
    self->name, self->pos);

  /* remove regions */
  for (int i = 0; i < self->num_lanes; i++)
    {
      track_lane_free (self->lanes[i]);
    }

  /* remove automation points, curves, tracks,
   * lanes*/
  automation_tracklist_free_members (
    &self->automation_tracklist);

  /* remove chords */
  for (int i = 0; i < self->num_chord_regions; i++)
    {
      arranger_object_free (
        (ArrangerObject *) self->chord_regions[i]);
      self->chord_regions[i] = NULL;
    }

  if (self->bpm_port)
    {
      port_disconnect_all (self->bpm_port);
      object_free_w_func_and_null (
        port_free, self->bpm_port);
    }
  if (self->time_sig_port)
    {
      port_disconnect_all (self->time_sig_port);
      object_free_w_func_and_null (
        port_free, self->time_sig_port);
    }

#undef _FREE_TRACK

  if (self->channel)
    {
      /* remove automation tracks - they are
       * already free'd by now */
      remove_ats_from_automation_tracklist (
        self, F_NO_PUBLISH_EVENTS);
      object_free_w_func_and_null (
        track_processor_free, self->processor);
      channel_free (self->channel);
    }

  if (self->widget &&
      GTK_IS_WIDGET (self->widget))
    gtk_widget_destroy (
      GTK_WIDGET (self->widget));

  g_free_and_null (self->name);
  g_free_and_null (self->comment);

  object_zero_and_free (self);

  g_message ("done");
}
